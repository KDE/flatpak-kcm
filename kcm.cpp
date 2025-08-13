/**
 * SPDX-FileCopyrightText: 2022 Suhaas Joshi <joshiesuhaas0@gmail.com>
 * SPDX-FileCopyrightText: 2023 ivan tkachenko <me@ratijas.tk>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kcm.h"

#include "permissionitem.h"
#include "permissionstore.h"
#include "restoredatamodel.h"

#include <KLocalizedString>
#include <KPluginFactory>
#include <QFile>
#include <algorithm>

K_PLUGIN_CLASS_WITH_JSON(KCMFlatpak, "kcm_flatpak.json")

using namespace Qt::StringLiterals;

KCMFlatpak::KCMFlatpak(QObject *parent, const KPluginMetaData &data, const QVariantList &args)
    : KQuickManagedConfigModule(parent, data)
    , m_refsModel(new FlatpakReferencesModel(this))
    , m_permissionStore(PermissionStore::instance())

{
    constexpr const char *uri = "org.kde.plasma.kcm.flatpakpermissions";

    qmlRegisterUncreatableType<KCMFlatpak>(uri, 1, 0, "KCMFlatpak", QString());
    qmlRegisterUncreatableType<FlatpakReference>(uri, 1, 0, "FlatpakReference", QStringLiteral("Should be obtained from a FlatpakReferencesModel"));
    qmlRegisterType<FlatpakPermissionModel>(uri, 1, 0, "FlatpakPermissionModel");
    qmlRegisterUncreatableType<FlatpakReferencesModel>(uri, 1, 0, "FlatpakReferencesModel", QStringLiteral("For enum access only"));
    qmlRegisterUncreatableType<FlatpakPermissionsSectionType>(uri, 1, 0, "FlatpakPermissionsSectionType", QStringLiteral("For enum access only"));

    qmlRegisterType<PermissionItem>(uri, 1, 0, "PermissionItem");
    qmlRegisterType<RemoteDesktopSessionsModel>(uri, 1, 0, "RemoteDesktopSessionsModel");
    qmlRegisterType<ScreencastSessionsModel>(uri, 1, 0, "ScreencastSessionsModel");
    m_permissionStore->loadTable("screenshot"_L1);
    m_permissionStore->loadTable("location"_L1);
    m_permissionStore->loadTable("notifications"_L1);
    m_permissionStore->loadTable("gamemode"_L1);
    m_permissionStore->loadTable("realtime"_L1);
    m_permissionStore->loadTable("devices"_L1); // For camera
    m_permissionStore->loadTable("remote-desktop"_L1);
    m_permissionStore->loadTable("screencast"_L1);
    m_permissionStore->loadTable("kde-authorized"_L1);

    connect(m_refsModel, &FlatpakReferencesModel::needsLoad, this, &KCMFlatpak::load);
    connect(m_refsModel, &FlatpakReferencesModel::settingsChanged, this, &KCMFlatpak::settingsChanged);

    const auto maybeRequestedIndex = indexFromArgs(args);

    if (maybeRequestedIndex) {
        m_index = *maybeRequestedIndex;
    }

    connect(this, &KQuickConfigModule::activationRequested, this, [this](const QVariantList &args) {
        const auto maybeRequestedIndex = indexFromArgs(args);

        if (maybeRequestedIndex) {
            m_index = *maybeRequestedIndex;
            Q_EMIT indexChanged(m_index);
        }
    });

    settingsChanged(); // Initialize Reset & Defaults buttons
}

std::optional<int> KCMFlatpak::indexFromArgs(const QVariantList &args) const
{
    if (args.isEmpty()) {
        return std::nullopt;
    }

    QString requestedReference;

    const QVariant &arg0 = args.at(0);
    if (arg0.canConvert<QString>()) {
        requestedReference = arg0.toString();
    } else {
        return std::nullopt;
    }

    const auto &refs = m_refsModel->references();
    const auto it = std::find_if(refs.constBegin(), refs.constEnd(), [&](FlatpakReference *ref) {
        return ref->ref() == requestedReference;
    });
    if (it != refs.constEnd()) {
        const auto index = std::distance(refs.constBegin(), it);
        return index;
    } else {
        return std::nullopt;
    }
}

void KCMFlatpak::load()
{
    m_refsModel->load(m_index);
    setNeedsSave(false);
}

void KCMFlatpak::save()
{
    m_refsModel->save(m_index);
}

void KCMFlatpak::defaults()
{
    m_refsModel->defaults(m_index);
}

bool KCMFlatpak::isSaveNeeded() const
{
    return m_refsModel->isSaveNeeded(m_index);
}

bool KCMFlatpak::isDefaults() const
{
    return m_refsModel->isDefaults(m_index);
}

void KCMFlatpak::setIndex(int index)
{
    if (m_index == index) {
        return;
    }
    m_index = index;
    Q_EMIT appIndexChanged();
    settingsChanged(); // Because Apply, Reset & Defaults buttons depend on m_index.
}

int KCMFlatpak::appIndex() const
{
    return m_index;
}

const AppsModel *KCMFlatpak::appsModel() const
{
    return &m_appsModel;
}

#include "kcm.moc"

#include "moc_kcm.cpp"
