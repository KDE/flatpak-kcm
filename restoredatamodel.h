/*
    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
    SPDX-FileCopyrightText: 2025 David Redondo <kde@david-redondo.de>
*/

#pragma once

#include <QAbstractListModel>
#include <QIcon>
#include <QQmlParserStatus>

#include "permissionstore.h"

struct DisplayedPermission {
    Q_GADGET
    Q_PROPERTY(QIcon icon MEMBER icon CONSTANT)
    Q_PROPERTY(QString text MEMBER text CONSTANT)
public:
    QIcon icon;
    QString text;
};

class RestoreDataModel : public QAbstractListModel, public QQmlParserStatus
{
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)
    Q_PROPERTY(QString appId MEMBER appId REQUIRED)
    Q_PROPERTY(int rowCount READ rowCount NOTIFY rowCountChanged)
public:
    enum Roles {
        IdRole = Qt::UserRole,
        DataRole
    };
    QString table;
    QString appId;

    virtual QList<DisplayedPermission> dataToDisplay(const QVariantMap &data) const = 0;

    QHash<int, QByteArray> roleNames() const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    bool hasChildren(const QModelIndex &index = {}) const override
    {
        return QAbstractItemModel::hasChildren(index);
    }
    Q_INVOKABLE void revoke(const QString &id) const;
Q_SIGNALS:
    void rowCountChanged();

private:
    struct Data {
        QString id;
        QVariant data;
        QList<DisplayedPermission> display;
    };
    void maybeAddEntry(const QString &id, const QVariant &data);
    void classBegin() override
    {
    }
    void componentComplete() override;

    std::shared_ptr<PermissionStore> permissionStore = PermissionStore::instance();
    QList<Data> m_entries;
    bool complete = false;
};

class RemoteDesktopSessionsModel : public RestoreDataModel
{
public:
    RemoteDesktopSessionsModel();
    QList<DisplayedPermission> dataToDisplay(const QVariantMap &data) const override;
};

class ScreencastSessionsModel : public RestoreDataModel
{
public:
    ScreencastSessionsModel();
    QList<DisplayedPermission> dataToDisplay(const QVariantMap &data) const override;
};
