/**
 * SPDX-FileCopyrightText: 2022 Suhaas Joshi <joshiesuhaas0@gmail.com>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "flatpakpermission.h"

#include <KLocalizedString>
#include <QTemporaryFile>
#include <KDesktopFile>
#include <KConfigGroup>

FlatpakPermission::FlatpakPermission(QString name, QString category, QString description, QString defaultValue, QStringList possibleValues, QString currentValue, ValueType type)
    : m_name(name),
      m_category(category),
      m_description(description),
      m_defaultValue(defaultValue),
      m_possibleValues(possibleValues),
      m_currentValue(currentValue),
      m_type(type)
{
}

QString FlatpakPermission::name() const
{
    return m_name;
}

QString FlatpakPermission::currentValue() const
{
    return m_currentValue;
}

QString FlatpakPermission::category() const
{
    return m_category;
}

QString FlatpakPermission::description() const
{
    return m_description;
}

QString FlatpakPermission::defaultValue() const
{
    return m_defaultValue;
}

QStringList FlatpakPermission::possibleValues() const
{
    return m_possibleValues;
}

FlatpakPermission::ValueType FlatpakPermission::type() const
{
    return m_type;
}

FlatpakPermissionModel::FlatpakPermissionModel(QObject *parent, QByteArray metadata) : QAbstractListModel(parent)
{
    QString name, category, description, defaultValue;
    QStringList possibleValues;

    QTemporaryFile f;
    if(!f.open()) {
        return;
    }
    f.write(metadata);
    f.close();

    KDesktopFile parser(f.fileName());
    const KConfigGroup contextGroup = parser.group("Context");;

    /* SHARED category */
    category = i18n("Subsystems Shared");
    const QString sharedPerms = contextGroup.readEntry("shared", QString());
    possibleValues << QStringLiteral("ON") << QStringLiteral("OFF");

    name = i18n("network");
    description = i18n("Internet Connection");
    defaultValue = sharedPerms.contains(name) ? QStringLiteral("ON") : QStringLiteral("OFF");
    m_permissions.append(FlatpakPermission(name, category, description, defaultValue, possibleValues));

    name = i18n("ipc");
    description = i18n("Inter-process Communication");
    defaultValue = sharedPerms.contains(name) ? QStringLiteral("ON") : QStringLiteral("OFF");
    m_permissions.append(FlatpakPermission(name, category, description, defaultValue, possibleValues));
    /* SHARED category */

    /* SOCKETS category */
    category = i18n("Sockets");
    const QString socketPerms = contextGroup.readEntry("sockets", QString());

    name = i18n("x11");
    description = i18n("X11 Windowing System");
    defaultValue = socketPerms.contains(name) ? QStringLiteral("ON") : QStringLiteral("OFF");
    m_permissions.append(FlatpakPermission(name, category, description, defaultValue, possibleValues));

    name = i18n("wayland");
    description = i18n("Wayland Windowing System");
    defaultValue = socketPerms.contains(name) ? QStringLiteral("ON") : QStringLiteral("OFF");
    m_permissions.append(FlatpakPermission(name, category, description, defaultValue, possibleValues));

    name = i18n("fallback-x11");
    description = i18n("Fallback to X11 Windowing System");
    defaultValue = socketPerms.contains(name) ? QStringLiteral("ON") : QStringLiteral("OFF");
    m_permissions.append(FlatpakPermission(name, category, description, defaultValue, possibleValues));

    name = i18n("pulseaudio");
    description = i18n("Pulseaudio Sound Server");
    defaultValue = socketPerms.contains(name) ? QStringLiteral("ON") : QStringLiteral("OFF");
    m_permissions.append(FlatpakPermission(name, category, description, defaultValue, possibleValues));

    name = i18n("session-bus");
    description = i18n("Session Bus Access");
    defaultValue = socketPerms.contains(name) ? QStringLiteral("ON") : QStringLiteral("OFF");
    m_permissions.append(FlatpakPermission(name, category, description, defaultValue, possibleValues));

    name = i18n("system-bus");
    description = i18n("System Bus Access");
    defaultValue = socketPerms.contains(name) ? QStringLiteral("ON") : QStringLiteral("OFF");
    m_permissions.append(FlatpakPermission(name, category, description, defaultValue, possibleValues));

    name = i18n("ssh-auth");
    description = i18n("Remote Login Access");
    defaultValue = socketPerms.contains(name) ? QStringLiteral("ON") : QStringLiteral("OFF");
    m_permissions.append(FlatpakPermission(name, category, description, defaultValue, possibleValues));

    name = i18n("pcsc");
    description = i18n("Smart Card Access");
    defaultValue = socketPerms.contains(name) ? QStringLiteral("ON") : QStringLiteral("OFF");
    m_permissions.append(FlatpakPermission(name, category, description, defaultValue, possibleValues));

    name = i18n("cups");
    description = i18n("Print System Access");
    defaultValue = socketPerms.contains(name) ? QStringLiteral("ON") : QStringLiteral("OFF");
    m_permissions.append(FlatpakPermission(name, category, description, defaultValue, possibleValues));
    /* SOCKETS category */

    /* DEVICES category */
    category = i18n("Device Access");
    const QString devicesPerms = contextGroup.readEntry("devices", QString());

    name = i18n("kvm");
    description = i18n("Kernel-based Virtual Machine Access");
    defaultValue = devicesPerms.contains(name) ? QStringLiteral("ON") : QStringLiteral("OFF");
    m_permissions.append(FlatpakPermission(name, category, description, defaultValue, possibleValues));

    name = i18n("dri");
    description = i18n("Direct Graphic Rendering");
    defaultValue = devicesPerms.contains(name) ? QStringLiteral("ON") : QStringLiteral("OFF");
    m_permissions.append(FlatpakPermission(name, category, description, defaultValue, possibleValues));

    name = i18n("shm");
    description = i18n("Host dev/shm");
    defaultValue = devicesPerms.contains(name) ? QStringLiteral("ON") : QStringLiteral("OFF");
    m_permissions.append(FlatpakPermission(name, category, description, defaultValue, possibleValues));

    name = i18n("all");
    description = i18n("Device Access");
    defaultValue = devicesPerms.contains(name) ? QStringLiteral("ON") : QStringLiteral("OFF");
    m_permissions.append(FlatpakPermission(name, category, description, defaultValue, possibleValues));
    /* DEVICES category */

    /* FEATURES category */
    category = i18n("Features Allowed");
    const QString featuresPerms = contextGroup.readEntry("features", QString());

    name = i18n("devel");
    description = i18n("System Calls by Development Tools");
    defaultValue = featuresPerms.contains(name) ? QStringLiteral("ON") : QStringLiteral("OFF");
    m_permissions.append(FlatpakPermission(name, category, description, defaultValue, possibleValues));

    name = i18n("multiarch");
    description = i18n("Run Multiarch/Multilib Binaries");
    defaultValue = featuresPerms.contains(name) ? QStringLiteral("ON") : QStringLiteral("OFF");
    m_permissions.append(FlatpakPermission(name, category, description, defaultValue, possibleValues));

    name = i18n("bluetooth");
    description = i18n("Bluetooth");
    defaultValue = featuresPerms.contains(name) ? QStringLiteral("ON") : QStringLiteral("OFF");
    m_permissions.append(FlatpakPermission(name, category, description, defaultValue, possibleValues));

    name = i18n("canbus");
    description = i18n("Canbus Socket Access");
    defaultValue = featuresPerms.contains(name) ? QStringLiteral("ON") : QStringLiteral("OFF");
    m_permissions.append(FlatpakPermission(name, category, description, defaultValue, possibleValues));

    name = i18n("per-app-dev-shm");
    description = i18n("Share dev/shm across all instances of an app per user ID");
    defaultValue = featuresPerms.contains(name) ? QStringLiteral("ON") : QStringLiteral("OFF");
    m_permissions.append(FlatpakPermission(name, category, description, defaultValue, possibleValues));
    /* FEATURES category */

    /* FILESYSTEM category */
    category = i18n("Filesystem Access");
    const QString fileSystemPerms = contextGroup.readEntry("filesystems", QString());
    const auto dirs = QStringView(fileSystemPerms).split(QLatin1Char(';'), Qt::SkipEmptyParts);

    QString homeVal, hostVal, hostOsVal, hostEtcVal;
    homeVal = hostVal = hostOsVal = hostEtcVal = i18n("OFF");
    possibleValues.clear();
    possibleValues << QStringLiteral("read-only") << QStringLiteral("read/write") << QStringLiteral("create");

    for(const QStringView &dir : dirs) {
        if (dir == QLatin1String("xdg-config/kdeglobals:ro")) {
            continue;
        }
        int sep = dir.lastIndexOf(QLatin1Char(':'));
        const QStringView postfix = sep > 0 ? dir.mid(sep) : QStringView();
        const QStringView symbolicName = dir.left(sep);

        if(symbolicName == QStringLiteral("home")) {
            if(postfix == QStringLiteral("ro")) {
                homeVal = i18n("read-only");
            } else if (postfix == QStringLiteral("create")) {
                homeVal = i18n("create");
            } else {
                homeVal = i18n("read/write");
            }
        } else if(symbolicName == QStringLiteral("host")) {
            if(postfix == QStringLiteral("ro")) {
                hostVal = i18n("read-only");
            } else if (postfix == QStringLiteral("create")) {
                hostVal = i18n("create");
            } else {
                hostVal = i18n("read/write");
            }
        } else if(symbolicName == QStringLiteral("host-os")) {
            if(postfix == QStringLiteral("ro")) {
                hostOsVal = i18n("read-only");
            } else if (postfix == QStringLiteral("create")) {
                hostOsVal = i18n("create");
            } else {
                hostOsVal = i18n("read/write");
            }
        } else if(symbolicName == QStringLiteral("host-etc")) {
            if(postfix == QStringLiteral("ro")) {
                hostEtcVal = i18n("read-only");
            } else if (postfix == QStringLiteral("create")) {
                hostEtcVal = i18n("create");
            } else {
                hostEtcVal = i18n("read/write");
            }
        } else {
            continue;
        }
    }

    name = i18n("home");
    description = i18n("Home Folder");
    possibleValues.removeAll(homeVal);
    if(homeVal != QStringLiteral("OFF")) {
        possibleValues.prepend(homeVal);
    }
    m_permissions.append(FlatpakPermission(name, category, description, homeVal, possibleValues, defaultValue, FlatpakPermission::ValueType::Complex));

    name = i18n("host");
    description = i18n("All System Files");
    possibleValues.removeAll(hostVal);
    if(hostVal != QStringLiteral("OFF")) {
        possibleValues.prepend(hostVal);
    }
    m_permissions.append(FlatpakPermission(name, category, description, hostVal, possibleValues, defaultValue, FlatpakPermission::ValueType::Complex));

    name = i18n("host-os");
    description = i18n("System Libraries, Executables and Binaries");
    possibleValues.removeAll(hostOsVal);
    if(hostOsVal != QStringLiteral("OFF")) {
        possibleValues.prepend(hostOsVal);
    }
    m_permissions.append(FlatpakPermission(name, category, description, hostOsVal, possibleValues, defaultValue, FlatpakPermission::ValueType::Complex));

    name = i18n("host-etc");
    description = i18n("System Configurations");
    possibleValues.removeAll(hostEtcVal);
    if(hostEtcVal != QStringLiteral("OFF")) {
        possibleValues.prepend(hostEtcVal);
    }
    m_permissions.append(FlatpakPermission(name, category, description, hostEtcVal, possibleValues, defaultValue, FlatpakPermission::ValueType::Complex));

    /* FILESYSTEM category */
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
    case Roles::ValueList:
        return m_permissions.at(index.row()).possibleValues();
    case Roles::CurrentValue:
        return m_permissions.at(index.row()).currentValue();
    case Roles::IsGranted:
        //this should be currentValue(), not defaultValue(), but I haven't implemented it yet
        return m_permissions.at(index.row()).defaultValue() != QStringLiteral("OFF");
    case Roles::Type:
        return m_permissions.at(index.row()).type();
    case Roles::IsComplex:
        return m_permissions.at(index.row()).type() == FlatpakPermission::ValueType::Complex;
    }

    return QVariant();
}

QHash<int, QByteArray> FlatpakPermissionModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[Roles::Name] = "name";
    roles[Roles::Category] = "category";
    roles[Roles::Description] = "description";
    roles[Roles::ValueList] = "valueList";
    roles[Roles::CurrentValue] = "currentValue";
    roles[Roles::IsGranted] = "isGranted";
    roles[Roles::Type] = "valueType";
    roles[Roles::IsComplex] = "isComplex";
    return roles;
}
