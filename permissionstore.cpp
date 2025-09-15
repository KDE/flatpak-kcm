/*
    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
    SPDX-FileCopyrightText: 2025 David Redondo <kde@david-redondo.de>
*/

#include "permissionstore.h"

#include <permissionstoreinterface.h>

#include <QDBusPendingCallWatcher>

using namespace Qt::StringLiterals;

PermissionStore::PermissionStore(Key)
    : m_permissionStoreInterface(std::make_unique<OrgFreedesktopImplPortalPermissionStoreInterface>("org.freedesktop.impl.portal.PermissionStore"_L1,
                                                                                                    "/org/freedesktop/impl/portal/PermissionStore"_L1,
                                                                                                    QDBusConnection::sessionBus()))
{
    qDBusRegisterMetaType<QMap<QString, QStringList>>();
    connect(m_permissionStoreInterface.get(), &OrgFreedesktopImplPortalPermissionStoreInterface::Changed, this, &PermissionStore::permissionsChanged);
}

PermissionStore::~PermissionStore() = default;

void PermissionStore::loadTable(const QString &table)
{
    if (m_tables.contains(table)) {
        return;
    }
    QDBusPendingReply<QStringList> listResourcesCall = m_permissionStoreInterface->List(table);
    connect(new QDBusPendingCallWatcher(listResourcesCall), &QDBusPendingCallWatcher::finished, this, [table, this](QDBusPendingCallWatcher *watcher) {
        watcher->deleteLater();
        if (watcher->isError()) {
            qWarning() << watcher->error();
            return;
        }
        const QStringList resources = decltype(listResourcesCall)(*watcher).value();
        for (const auto &resource : resources) {
            QDBusPendingReply<QMap<QString, QStringList>, QDBusVariant> lookupCall = m_permissionStoreInterface->Lookup(table, resource);
            connect(new QDBusPendingCallWatcher(lookupCall),
                    &QDBusPendingCallWatcher::finished,
                    this,
                    [table, resource, this](QDBusPendingCallWatcher *watcher) {
                        watcher->deleteLater();
                        if (watcher->isError()) {
                            qWarning() << watcher->error();
                            return;
                        }
                        const auto reply = decltype(lookupCall)(*watcher);
                        const auto permissions = reply.argumentAt<0>();
                        m_tables[table].insert(resource,
                                               Entry{.data = reply.argumentAt<1>().variant(), .permissions = {permissions.cbegin(), permissions.cend()}});
                        Q_EMIT dataChanged(table, resource, reply.argumentAt<1>().variant());
                        Q_EMIT permissionChanged(table, resource);
                    });
        }
    });
}

void PermissionStore::permissionsChanged(const QString &table,
                                         const QString &id,
                                         bool deleted,
                                         const QDBusVariant &data,
                                         const QMap<QString, QStringList> &permissions)
{
    if (!m_tables.contains(table)) {
        qDebug() << "Got permission change on unknown table" << table << ". Trying to load it.";
        loadTable(table);
        return;
    }
    if (deleted) {
        m_tables[table].remove(id);
        Q_EMIT resourceDeleted(table, id);
        return;
    }
    auto &oldEntry = m_tables[table][id];
    auto entry = Entry{data.variant(), {permissions.begin(), permissions.end()}};
    std::swap(oldEntry, entry);
    if (oldEntry.data != entry.data) {
        Q_EMIT dataChanged(table, id, entry.data);
    }
    if (oldEntry.permissions != entry.permissions) {
        Q_EMIT permissionChanged(table, id);
    }
}

QStringList PermissionStore::lookupPermission(const QString &table, const QString &resource, const QString &appId) const
{
    const auto _table = m_tables.find(table);
    if (_table == m_tables.end()) {
        return {};
    }
    const auto _resource = _table->find(resource);
    if (_resource == _table->end()) {
        return {};
    }
    return _resource->permissions.value(appId);
}

void PermissionStore::setPermission(const QString &table, const QString &resource, const QString &appId, const QStringList &permission) const
{
    auto pendingCall = m_permissionStoreInterface->SetPermission(table, true, resource, appId, permission);
    connect(new QDBusPendingCallWatcher(pendingCall), &QDBusPendingCallWatcher::finished, this, [](QDBusPendingCallWatcher *watcher) {
        watcher->deleteLater();
        if (watcher->isError()) {
            qWarning() << watcher->error();
        }
    });
}

QList<std::pair<QString, QVariant>> PermissionStore::allDataForApp(const QString &table, const QString &appId) const
{
    QList<std::pair<QString, QVariant>> result;
    const auto _table = m_tables.find(table);
    if (_table == m_tables.end()) {
        return result;
    }
    for (const auto &[id, entry] : _table->asKeyValueRange()) {
        if (entry.permissions.contains(appId)) {
            result.emplace_back(id, entry.data);
        }
    }
    return result;
}

void PermissionStore::deleteResource(const QString &table, const QString &resource) const
{
    auto pendingCall = m_permissionStoreInterface->Delete(table, resource);
    connect(new QDBusPendingCallWatcher(pendingCall), &QDBusPendingCallWatcher::finished, this, [](QDBusPendingCallWatcher *watcher) {
        watcher->deleteLater();
        if (watcher->isError()) {
            qWarning() << watcher->error();
        }
    });
}

#include "moc_permissionstore.cpp"
