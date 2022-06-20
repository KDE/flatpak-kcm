/**
 * SPDX-FileCopyrightText: 2022 Suhaas Joshi <joshiesuhaas0@gmail.com>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "flatpakpermission.h"

#include <KLocalizedString>

FlatpakPermission::FlatpakPermission(QString name, QString category, QString description, QString value, ValueType type)
    : m_name(name),
      m_category(category),
      m_description(description),
      m_value(value),
      m_type(type)
{
}

QString FlatpakPermission::name() const
{
    return m_name;
}

QString FlatpakPermission::value() const
{
    return m_value;
}

QString FlatpakPermission::category() const
{
    return m_category;
}

QString FlatpakPermission::description() const
{
    return m_description;
}

FlatpakPermission::ValueType FlatpakPermission::type() const
{
    return m_type;
}

FlatpakPermissionModel::FlatpakPermissionModel(QObject *parent) : QAbstractListModel(parent)
{
    QString category;
    QString name;
    QString description;

    /* SHARED category */
    category = i18n("shared");

    name = i18n("network");
    description = i18n("Internet Connection");
    m_permissions.append(FlatpakPermission(name, category, description));

    name = i18n("ipc");
    description = i18n("Inter-process Communication");
    m_permissions.append(FlatpakPermission(name, category, description));
    /* SHARED category */

    /* SOCKETS category */
    name = i18n("x11");
    description = i18n("X11 Windowing System");
    m_permissions.append(FlatpakPermission(name, category, description));

    name = i18n("wayland");
    description = i18n("Wayland Windowing System");
    m_permissions.append(FlatpakPermission(name, category, description));

    name = i18n("fallback-x11");
    description = i18n("Fallback to X11 Windowing System");
    m_permissions.append(FlatpakPermission(name, category, description));

    name = i18n("pulseaudio");
    description = i18n("Pulseaudio Sound Server");
    m_permissions.append(FlatpakPermission(name, category, description));

    name = i18n("session-bus");
    description = i18n("Session Bus Access");
    m_permissions.append(FlatpakPermission(name, category, description));

    name = i18n("system-bus");
    description = i18n("System Bus Access");
    m_permissions.append(FlatpakPermission(name, category, description));

    name = i18n("ssh-auth");
    description = i18n("Remote Login Access");
    m_permissions.append(FlatpakPermission(name, category, description));

    name = i18n("pcsc");
    description = i18n("Smart Card Access");
    m_permissions.append(FlatpakPermission(name, category, description));

    name = i18n("cups");
    description = i18n("Print System Access");
    m_permissions.append(FlatpakPermission(name, category, description));
    /* SOCKETS category */

    /* DEVICES category */
    category = i18n("devices");

    name = i18n("kvm");
    description = i18n("Kernel-based Virtual Machine Access");
    m_permissions.append(FlatpakPermission(name, category, description));

    name = i18n("all");
    description = i18n("Device Access");
    m_permissions.append(FlatpakPermission(name, category, description));
    /* DEVICES category */

    /* FEATURES category */
    category = i18n("features");

    name = i18n("devel");
    description = i18n("System Calls by Development Tools");
    m_permissions.append(FlatpakPermission(name, category, description));

    name = i18n("multiarch");
    description = i18n("Run Multiarch/Multilib Binaries");
    m_permissions.append(FlatpakPermission(name, category, description));

    name = i18n("bluetooth");
    description = i18n("Bluetooth");
    m_permissions.append(FlatpakPermission(name, category, description));

    name = i18n("canbus");
    description = i18n("Canbus Socket Access");
    m_permissions.append(FlatpakPermission(name, category, description));

    name = i18n("per-app-dev-shm");
    description = i18n("Share dev/shm across all instances of an app per user ID");
    m_permissions.append(FlatpakPermission(name, category, description));
    /* FEATURES category */

 }
int FlatpakPermissionModel::rowCount(const QModelIndex &parent) const
{
    if(parent.isValid()) {
        return 0;
    }
    return m_permissions.count();
}

QVariant FlatpakPermissionModel::data(const QModelIndex &index, int role) const
{
    if(!index.isValid()) {
        return QVariant();
    }

    switch(role) {
    case Roles::Name:
        return m_permissions.at(index.row()).name();
    case Roles::Category:
        return m_permissions.at(index.row()).category();
    case Roles::Description:
        return m_permissions.at(index.row()).description();
    case Roles::Value:
        return m_permissions.at(index.row()).value();
    case Roles::Type:
        return m_permissions.at(index.row()).type();
    }

    return QVariant();
}

QHash<int, QByteArray> FlatpakPermissionModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[Roles::Name] = "name";
    roles[Roles::Category] = "category";
    roles[Roles::Description] = "description";
    roles[Roles::Value] = "value";
    roles[Roles::Type] = "valueType";
    return roles;
}
