/*
    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
    SPDX-FileCopyrightText: 2025 David Redondo <kde@david-redondo.de>
*/

#include "permissionitem.h"

#include "permissionstore.h"

using namespace Qt::StringLiterals;

PermissionItem::PermissionItem() = default;

void PermissionItem::componentComplete()
{
    complete = true;
    connect(permissionStore.get(), &PermissionStore::permissionChanged, this, [this](const QString &table, const QString &resource) {
        if (table == this->table && resource == this->resource) {
            Q_EMIT permissionsChanged();
        }
    });
    Q_EMIT permissionsChanged();
}

QStringList PermissionItem::permissions() const
{
    if (!complete) {
        return {};
    }
    return permissionStore->lookupPermission(table, resource, appId);
}

void PermissionItem::setPermissions(const QStringList &permissions)
{
    permissionStore->setPermission(table, resource, appId, permissions);
}

#include "moc_permissionitem.cpp"
