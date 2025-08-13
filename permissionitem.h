/*
    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
    SPDX-FileCopyrightText: 2025 David Redondo <kde@david-redondo.de>
*/

#pragma once

#include <QObject>
#include <QQmlParserStatus>

#include "permissionstore.h"

class PermissionItem : public QObject, public QQmlParserStatus
{
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)
    Q_PROPERTY(QString table MEMBER table REQUIRED)
    Q_PROPERTY(QString resource MEMBER resource REQUIRED)
    Q_PROPERTY(QString appId MEMBER appId REQUIRED)
    Q_PROPERTY(QStringList permissions READ permissions WRITE setPermissions NOTIFY permissionsChanged)
public:
    PermissionItem();
    QString table;
    QString resource;
    QString appId;
    QStringList permissions() const;
    void setPermissions(const QStringList &permissions);
Q_SIGNALS:
    void permissionsChanged();

private:
    std::shared_ptr<PermissionStore> permissionStore = PermissionStore::instance();
    void classBegin() override
    {
    }
    void componentComplete() override;
    bool complete = false;
};
