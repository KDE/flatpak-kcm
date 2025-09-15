/*
    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
    SPDX-FileCopyrightText: 2025 David Redondo <kde@david-redondo.de>
*/

#include "restoredatamodel.h"

using namespace Qt::StringLiterals;

#include <KLocalizedString>
#include <KService>

#include <QDBusArgument>
#include <QGuiApplication>
#include <QLocale>
#include <QRect>
#include <QRegularExpression>
#include <QScreen>

void RestoreDataModel::componentComplete()
{
    complete = true;

    auto removeEntry = [this](const QString &id) {
        const auto it = std::ranges::find(std::as_const(m_entries), id, &Data::id);
        if (it == std::ranges::end(m_entries)) {
            return;
        }
        const auto index = std::ranges::distance(m_entries.begin(), it);
        beginRemoveRows(QModelIndex(), index, index);
        m_entries.erase(it);
        endRemoveRows();
    };

    connect(permissionStore.get(), &PermissionStore::resourceDeleted, this, [this, removeEntry](const QString &table, const QString &resource) {
        if (table != this->table) {
            return;
        }
        removeEntry(resource);
        Q_EMIT rowCountChanged();
    });
    connect(permissionStore.get(),
            &PermissionStore::dataChanged,
            this,
            [this, removeEntry](const QString &table, const QString &resource, const QVariant newData) {
                if (table != this->table) {
                    return;
                }
                removeEntry(resource);
                maybeAddEntry(resource, newData);
                Q_EMIT rowCountChanged();
            });

    const auto data = permissionStore->allDataForApp(table, appId);
    for (const auto &[resource, data] : data) {
        maybeAddEntry(resource, data);
    }
    if (m_entries.size() > 0) {
        Q_EMIT rowCountChanged();
    }
}

int RestoreDataModel::rowCount([[maybe_unused]] const QModelIndex &parent) const
{
    return m_entries.size();
}

QHash<int, QByteArray> RestoreDataModel::roleNames() const
{
    return {
        {Qt::DisplayRole, "display"_ba},
        {IdRole, "id"_ba},
        {DataRole, "data"_ba},
    };
}

QVariant RestoreDataModel::data(const QModelIndex &index, int role) const
{
    if (!checkIndex(index, CheckIndexOption::IndexIsValid | CheckIndexOption::ParentIsInvalid)) {
        return {};
    }
    const auto &entry = m_entries.at(index.row());
    switch (role) {
    case Qt::DisplayRole:
        return QVariant::fromValue(entry.display);
    case IdRole:
        return entry.id;
    case DataRole:
        return entry.data;
    }
    return QVariant();
}

void RestoreDataModel::maybeAddEntry(const QString &id, const QVariant &data)
{
    // KDE restore data format
    // (s    u          v                     )
    // ("KDE" version=1 serialized QVariantMap)
    const auto argument = data.value<QDBusArgument>();
    if (argument.currentSignature() != "(suv)"_L1) {
        return;
    }
    argument.beginStructure();
    const auto kde = qdbus_cast<QString>(argument);
    const auto version = qdbus_cast<unsigned int>(argument);
    if (kde != "KDE"_L1 || version != 1) {
        return;
    }
    const QByteArray bytes = qdbus_cast<QDBusVariant>(argument).variant().toByteArray();
    argument.endStructure();

    QVariantMap restoreData;
    QDataStream serializedMap(bytes);
    serializedMap >> restoreData;
    if (restoreData.isEmpty()) {
        return;
    }
    const auto displayData = dataToDisplay(restoreData);
    if (displayData.isEmpty()) {
        // This can happen if the impl switched format in the past, since this cannot be restore -> skip
        return;
    }
    beginInsertRows(QModelIndex(), m_entries.size(), m_entries.size());
    m_entries.emplace_back(id, restoreData, displayData);
    endInsertRows();
}

void RestoreDataModel::revoke(const QString &id) const
{
    if (std::any_of(m_entries.begin(), m_entries.end(), [id](const Data &data) {
            return data.id == id;
        })) {
        permissionStore->deleteResource(table, id);
    }
}

RemoteDesktopSessionsModel::RemoteDesktopSessionsModel()
{
    table = "remote-desktop"_L1;
}

QList<DisplayedPermission> RemoteDesktopSessionsModel::dataToDisplay(const QVariantMap &data) const
{
    QList<DisplayedPermission> powers;
    if (data.value("screenShareEnabled"_L1).toBool()) {
        powers.emplace_back(QIcon::fromTheme(u"media-record"_s), i18nc("not an action but the ability to do so", "Record the Screen"));
    }
    if (data.value("clipboardEnabled"_L1).toBool()) {
        powers.emplace_back(QIcon::fromTheme(u"edit_copy"_s), i18nc("not an action but the ability to do so", "Control the Clipboard"));
    }
    const int devices = data.value("devices"_L1).toInt();
    if (devices != 0) {
        QStringList deviceStrings;
        if (devices & 1) {
            deviceStrings += i18nc("Part of a list of things", "Keyboard");
        }
        if (devices & 2) {
            deviceStrings += i18nc("Part of a list of things", "Mouse");
        }
        if (devices & 4) {
            deviceStrings += i18nc("Part of a list of things", "Touchscreen");
        }
        powers.emplace_back(QIcon::fromTheme(u"dialog-input-devices"_s),
                            i18nc("%1 is a list of things formed from above", "Control %1", QLocale::system().createSeparatedList(deviceStrings)));
    }
    return powers;
}

// The screencast portal serializes a custom struct into the variant map, we need a type of the same
// name that deserializes the same so we can deserialize it
struct WindowRestoreInfo {
    QString appId;
    QString title;
};
QDataStream &operator<<(QDataStream &out, const WindowRestoreInfo &info)
{
    return out << info.appId << info.title;
}
QDataStream &operator>>(QDataStream &in, WindowRestoreInfo &info)
{
    return in >> info.appId >> info.title;
}
Q_DECLARE_METATYPE(WindowRestoreInfo)

ScreencastSessionsModel::ScreencastSessionsModel()
{
    table = "screencast"_L1;
    qRegisterMetaType<QList<WindowRestoreInfo>>();
}

QList<DisplayedPermission> ScreencastSessionsModel::dataToDisplay(const QVariantMap &data) const
{
    const auto region = data.value("region"_L1).value<QRect>();
    const auto restoreWindows = data.value("windows"_L1).value<QList<WindowRestoreInfo>>();
    auto outputs = data.value("outputs"_L1).toStringList();

    QList<DisplayedPermission> sources;

    for (const auto &window : restoreWindows) {
        auto app = KService::serviceByDesktopName(window.appId);
        sources.emplace_back(
            app ? QIcon::fromTheme(app->icon()) : QIcon::fromTheme(u"window"_s),
            i18nc("%1 is the title of a window, %2 the name of an application", "Window \"%1\" of %2", window.title, app ? app->name() : window.appId));
    }

    if (!region.isNull()) {
        sources.emplace_back(QIcon::fromTheme(u"region"_s), i18n("Region at (%1,%2) with size %3x%4", region.x(), region.y(), region.width(), region.height()));
    }
    outputs.removeAll("Region"_L1);

    const auto screens = qGuiApp->screens();

    if (outputs.removeIf([](const QString &output) {
            return output.startsWith("Virtual"_L1);
        })
        > 0) {
        sources.emplace_back(QIcon::fromTheme(u"monitor"_s), i18n("Virtual Screen"));
    }

    if (outputs.removeAll("Workspace"_L1) > 0) {
        sources.emplace_back(QIcon::fromTheme(u"virtual-desktops"_s), i18n("The entire Workspace"));
    }

    auto makeScreenName = [](const QScreen *screen) -> QString {
        return screen->manufacturer() + u" " + screen->model();
    };

    auto beginDisconnected = sources.end();
    QStringList disconnectedScreens;
    for (const auto &output : outputs) {
        auto it = std::ranges::find_if(screens, [output](const QScreen *screen) {
            const auto pos = screen->geometry().topLeft();
            return u"%1x%2"_s.arg(pos.x()).arg(pos.y()) == output;
        });
        if (it != screens.end()) {
            beginDisconnected = sources.emplace(beginDisconnected, QIcon::fromTheme(u"monitor"_s), makeScreenName(*it));
        } else {
            beginDisconnected = sources.emplace(beginDisconnected,
                                                QIcon::fromTheme(u"monitor"_s),
                                                i18nc("%1 is the position of the screen", "A disconnected Screen at %1", output));
        }
    }
    return sources;
}

#include "moc_restoredatamodel.cpp"
