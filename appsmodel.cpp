/*
    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
    SPDX-FileCopyrightText: 2025 David Redondo <kde@david-redondo.de>
*/

#include "appsmodel.h"

#include <KApplicationTrader>

using namespace Qt::StringLiterals;

AppsModel::AppsModel(QObject *parent)
{
    loadApps();
}

void AppsModel::loadApps()
{
    beginResetModel();
    m_apps = KApplicationTrader::query([](const KService::Ptr &app) {
        return !app->noDisplay();
    });
    endResetModel();
}

int AppsModel::rowCount(const QModelIndex &parent) const
{
    return m_apps.size();
}

QHash<int, QByteArray> AppsModel::roleNames() const
{
    return {
        {Qt::DisplayRole, "display"_ba},
        {Qt::DecorationRole, "decoration"_ba},
        {AppIdRole, "appId"_ba},
        {IsHostRole, "isHost"_ba},
    };
}

QVariant AppsModel::data(const QModelIndex &index, int role) const
{
    if (!checkIndex(index, CheckIndexOption::IndexIsValid | CheckIndexOption::ParentIsInvalid)) {
        return {};
    }

    auto sandboxId = [](const KService::Ptr &app) {
        if (const auto flatpak = app->property<QString>("X-Flatpak"_L1); !flatpak.isEmpty()) {
            return flatpak;
        }
        if (const auto snap = app->property<QString>("X-SnapInstanceName"_L1); !snap.isEmpty()) {
            return snap;
        }
        return QString();
    };

    const auto &app = m_apps.at(index.row());
    switch (role) {
    case Qt::DisplayRole:
        return app->name();
    case Qt::DecorationRole:
        return app->icon();
    case AppIdRole: {
        const auto id = sandboxId(app);
        return id.isEmpty() ? app->desktopEntryName() : id;
    }
    case IsHostRole:
        return sandboxId(app).isEmpty();
    }
    return {};
}

#include "moc_appsmodel.cpp"
