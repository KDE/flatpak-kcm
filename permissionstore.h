/*
    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
    SPDX-FileCopyrightText: 2025 David Redondo <kde@david-redondo.de>
*/

#pragma once

#include <QDBusVariant>
#include <QHash>
#include <QMap>
#include <QObject>
#include <QVariant>

#include <memory>

class OrgFreedesktopImplPortalPermissionStoreInterface;

class PermissionStore : public QObject
{
    Q_OBJECT
    // Public constructor with Key, so make_shared can be used but the normal but the constructor is
    // still inaccessible
    struct Key {
    };

public:
    PermissionStore(Key);
    ~PermissionStore() override;
    Q_DISABLE_COPY_MOVE(PermissionStore)

    static std::shared_ptr<PermissionStore> instance()
    {
        // A pseudo singleton, so it is alive as long as the kcm is open but not keep on living while
        // when user switches to another kcm.
        static std::weak_ptr<PermissionStore> weak_instance;
        std::shared_ptr<PermissionStore> instance = weak_instance.lock();
        if (!instance) {
            instance = std::make_shared<PermissionStore>(Key{});
            weak_instance = instance;
        }
        return instance;
    }
    void loadTable(const QString &table);

    struct Entry {
        QVariant data;
        QHash<QString, QStringList> permissions;
    };
    [[nodiscard]] QStringList lookupPermission(const QString &table, const QString &resource, const QString &appId) const;
    void setPermission(const QString &table, const QString &resource, const QString &appId, const QStringList &permission) const;
    [[nodiscard]] QList<std::pair<QString, QVariant>> allDataForApp(const QString &table, const QString &appId) const;
    void deleteResource(const QString &table, const QString &resource) const;
Q_SIGNALS:
    void resourceDeleted(const QString &table, const QString &resource);
    void permissionChanged(const QString &table, const QString &resource);
    void dataChanged(const QString &table, const QString &resource, const QVariant &newData);

private:
    using Table = QHash<QString, Entry>;
    QHash<QString, Table> m_tables;
    void permissionsChanged(const QString &table, const QString &id, bool deleted, const QDBusVariant &data, const QMap<QString, QStringList> &permissions);
    std::unique_ptr<OrgFreedesktopImplPortalPermissionStoreInterface> m_permissionStoreInterface;
};
