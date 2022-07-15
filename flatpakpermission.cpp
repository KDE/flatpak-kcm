/**
 * SPDX-FileCopyrightText: 2022 Suhaas Joshi <joshiesuhaas0@gmail.com>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "flatpakpermission.h"

#include <KLocalizedString>
#include <QTemporaryFile>
#include <QFileInfo>
#include <KDesktopFile>
#include <KConfigGroup>
#include <QDebug>

FlatpakPermission::FlatpakPermission(QString name, QString category, QString description, QString defaultValue, QStringList possibleValues, QString currentValue, ValueType type)
    : m_name(name),
      m_category(category),
      m_description(description),
      m_defaultValue(defaultValue),
      m_possibleValues(possibleValues),
      m_currentValue(defaultValue),
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

QString FlatpakPermission::fsCurrentValue() const
{
    if (m_currentValue == QStringLiteral("OFF")) {
        return QString();
    }

    QString val;
    if (m_currentValue == QStringLiteral("read-only")) {
        val = QStringLiteral("ro");
    } else if (m_currentValue == QStringLiteral("create")) {
        val = QStringLiteral("create");
    } else {
        val = QStringLiteral("rw");
    }
    return val;
}

//QString FlatpakPermission::fsDefaultValue() const
//{
//    QString val;
//    if (m_defaultValue == QStringLiteral("read-only")) {
//        val = QStringLiteral("ro");
//    } else if (m_defaultValue == QStringLiteral("create")) {
//        val = QStringLiteral("create");
//    } else if (m_defaultValue == QStringLiteral("OFF")) {
//        val = m_defaultValue;
//    } else {
//        val = QStringLiteral("rw");
//    }
//    return val;
//}

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

void FlatpakPermission::setCurrentValue(QString val)
{
//    if (m_type == Filesystems) {
//        if (val == QStringLiteral("ro")) {
//            val = QStringLiteral("read-only");
//        } else if (val == QStringLiteral("rw")) {
//            val = QStringLiteral("read/write");
//        }
//    }
    m_currentValue = val;
}

FlatpakPermissionModel::FlatpakPermissionModel(QObject *parent)
    : QAbstractListModel(parent)
{
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
    case Roles::CurrentValue:
        return m_permissions.at(index.row()).currentValue();
    case Roles::DefaultValue:
        return m_permissions.at(index.row()).defaultValue();
    case Roles::IsGranted:
        return m_permissions.at(index.row()).currentValue() != QStringLiteral("OFF");
    case Roles::Type:
        return m_permissions.at(index.row()).type();
    case Roles::IsSimple:
        return m_permissions.at(index.row()).type() == FlatpakPermission::ValueType::Simple;
    case Roles::ValueList:
        QStringList valuesTmp = m_permissions.at(index.row()).possibleValues();
        QString currentVal = m_permissions.at(index.row()).currentValue();
        valuesTmp.removeAll(currentVal);
        if (currentVal != QStringLiteral("OFF")) {
            valuesTmp.prepend(currentVal);
        }
        return valuesTmp;
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
    roles[Roles::DefaultValue] = "defaultValue";
    roles[Roles::IsGranted] = "isGranted";
    roles[Roles::Type] = "valueType";
    roles[Roles::IsSimple] = "isSimple";
    return roles;
}

void FlatpakPermissionModel::loadDefaultValues()
{
    QString name, category, description, defaultValue;
    QStringList possibleValues;

    const QByteArray metadata = m_reference->metadata();
    const QString path = m_reference->path();

    QTemporaryFile f;
    if(!f.open()) {
        return;
    }
    f.write(metadata);
    f.close();

    KDesktopFile parser(f.fileName());
    const KConfigGroup contextGroup = parser.group("Context");

    /* SHARED category */
    category = i18n("shared");
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
    category = i18n("sockets");
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
    category = i18n("devices");
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
    category = i18n("features");
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
    category = i18n("filesystems");
    const QString fileSystemPerms = contextGroup.readEntry("filesystems", QString());
    const auto dirs = QStringView(fileSystemPerms).split(QLatin1Char(';'), Qt::SkipEmptyParts);

    QString homeVal, hostVal, hostOsVal, hostEtcVal;
    homeVal = hostVal = hostOsVal = hostEtcVal = i18n("OFF");
    possibleValues.clear();
    possibleValues << QStringLiteral("read/write") << QStringLiteral("read-only") << QStringLiteral("create");

    QVector<FlatpakPermission> filesysTemp;

    for(const QStringView &dir : dirs) {
        if (dir == QLatin1String("xdg-config/kdeglobals:ro")) {
            continue;
        }
        int sep = dir.lastIndexOf(QLatin1Char(':'));
        const QStringView postfix = sep > 0 ? dir.mid(sep) : QStringView();
        const QStringView symbolicName = dir.left(sep);

        if(symbolicName == QStringLiteral("home")) {
            if(postfix == QStringLiteral(":ro")) {
                homeVal = i18n("read-only");
            } else if (postfix == QStringLiteral(":create")) {
                homeVal = i18n("create");
            } else {
                homeVal = i18n("read/write");
            }
        } else if(symbolicName == QStringLiteral("host")) {
            if(postfix == QStringLiteral(":ro")) {
                hostVal = i18n("read-only");
            } else if (postfix == QStringLiteral(":create")) {
                hostVal = i18n("create");
            } else {
                hostVal = i18n("read/write");
            }
        } else if(symbolicName == QStringLiteral("host-os")) {
            if(postfix == QStringLiteral(":ro")) {
                hostOsVal = i18n("read-only");
            } else if (postfix == QStringLiteral(":create")) {
                hostOsVal = i18n("create");
            } else {
                hostOsVal = i18n("read/write");
            }
        } else if(symbolicName == QStringLiteral("host-etc")) {
            if(postfix == QStringLiteral(":ro")) {
                hostEtcVal = i18n("read-only");
            } else if (postfix == QStringLiteral(":create")) {
                hostEtcVal = i18n("create");
            } else {
                hostEtcVal = i18n("read/write");
            }
        } else {
            name = symbolicName.toString();
            description = name;
            QString fsval = i18n("OFF");
            if(postfix == QStringLiteral(":ro")) {
                fsval = i18n("read-only");
            } else if(postfix == QStringLiteral(":create")) {
                fsval = i18n("create");
            } else {
                fsval = i18n("read/write");
            }
            filesysTemp.append(FlatpakPermission(name, category, description, fsval, possibleValues, fsval, FlatpakPermission::ValueType::Filesystems));
        }
    }

    name = i18n("home");
    description = i18n("Home Folder");
    possibleValues.removeAll(homeVal);
    m_permissions.append(FlatpakPermission(name, category, description, homeVal, possibleValues, homeVal, FlatpakPermission::ValueType::Filesystems));

    name = i18n("host");
    description = i18n("All System Files");
    possibleValues.removeAll(hostVal);
    m_permissions.append(FlatpakPermission(name, category, description, hostVal, possibleValues, hostVal, FlatpakPermission::ValueType::Filesystems));

    name = i18n("host-os");
    description = i18n("System Libraries, Executables and Binaries");
    possibleValues.removeAll(hostOsVal);
    m_permissions.append(FlatpakPermission(name, category, description, hostOsVal, possibleValues, hostOsVal, FlatpakPermission::ValueType::Filesystems));

    name = i18n("host-etc");
    description = i18n("System Configurations");
    possibleValues.removeAll(hostEtcVal);
    m_permissions.append(FlatpakPermission(name, category, description, hostEtcVal, possibleValues, hostEtcVal, FlatpakPermission::ValueType::Filesystems));

    m_permissions.append(filesysTemp);
    /* FILESYSTEM category */

    /* SESSION BUS category */
    category = i18n("Session Bus Policy");
    const KConfigGroup sessionBusGroup = parser.group("Session Bus Policy");
    possibleValues.clear();
    possibleValues << i18n("talk") << i18n("own") << i18n("see");
    if(sessionBusGroup.exists()) {
        const QMap<QString, QString> busMap = sessionBusGroup.entryMap();
        const QStringList busList = busMap.keys();
        for(int i = 0; i < busList.length(); ++i) {
            name = description = busList.at(i);
            defaultValue = busMap.value(busList.at(i));
            m_permissions.append(FlatpakPermission(name, category, description, defaultValue, possibleValues, defaultValue, FlatpakPermission::ValueType::Complex));
        }
    }
    /* SESSION BUS category */

    /* SYSTEM BUS category */
    category = i18n("System Bus Policy");
    const KConfigGroup systemBusGroup = parser.group("System Bus Policy");
    if(systemBusGroup.exists()) {
        const QMap<QString, QString> busMap = systemBusGroup.entryMap();
        const QStringList busList = busMap.keys();
        for(int i = 0; i < busList.length(); ++i) {
            name = description = busList.at(i);
            defaultValue = busMap.value(busList.at(i));
            m_permissions.append(FlatpakPermission(name, category, description, defaultValue, possibleValues, defaultValue, FlatpakPermission::ValueType::Complex));
        }
    }
    /* SYSTEM BUS category */

    /* ENVIRONMENT category */
    category = i18n("Environment");
    const KConfigGroup environmentGroup = parser.group("Environment");
    possibleValues.clear();
    if(environmentGroup.exists()) {
        const QMap<QString, QString> busMap = environmentGroup.entryMap();
        const QStringList busList = busMap.keys();
        for(int i = 0; i < busList.length(); ++i) {
            name = description = busList.at(i);
            defaultValue = busMap.value(busList.at(i));
            m_permissions.append(FlatpakPermission(name, category, description, defaultValue, possibleValues, defaultValue, FlatpakPermission::ValueType::Complex));
        }
    }
    /* ENVIRONMENT category */
    loadCurrentValues();

}

void FlatpakPermissionModel::loadCurrentValues()
{
    const QString path = m_reference->path();

    /* all permissions are at default, so nothing to load */
    if(!QFileInfo(path).exists()) {
        return;
    }

    KDesktopFile parser(path);
    const KConfigGroup contextGroup = parser.group("Context");

    int i;
    for(i = 0; i < m_permissions.length(); ++i) {
        FlatpakPermission *perm = &m_permissions[i];
        if(perm->category() == QStringLiteral("Session Bus Policy") || perm->category() == QStringLiteral("System Bus Policy")) {
            break;
        }
        const QString cat = contextGroup.readEntry(perm->category(), QString());
        if(perm->type() == FlatpakPermission::ValueType::Simple) {
            if(cat.contains(perm->name())) {
                if(perm->defaultValue() == QStringLiteral("ON")) {
                    perm->setCurrentValue(QStringLiteral("OFF"));
                } else {
                    perm->setCurrentValue(QStringLiteral("ON"));
                }
            }
        } else {
            if(cat.contains(perm->name())) {
                int permIndex = cat.indexOf(perm->name());
                /* the permission is just being set off, the access level isn't changed */
                if(permIndex > 0 && cat[permIndex - 1] == QLatin1Char('!')) {
                    perm->setCurrentValue(QStringLiteral("OFF"));
                    continue;
                }

                int valueIndex = cat.indexOf(QLatin1Char(':'), cat.indexOf(perm->name()));

                if(valueIndex == -1) {
                    perm->setCurrentValue(QStringLiteral("read/write"));
                    continue;
                }
                if(perm->category() == QStringLiteral("filesystems")) {
                    if(cat[valueIndex + 1] == QLatin1Char('r')) {
                        if(cat[valueIndex + 2] == QLatin1Char('w')) {
                            perm->setCurrentValue(QStringLiteral("read/write"));
                        } else {
                            perm->setCurrentValue(QStringLiteral("read-only"));
                        }
                    } else if(cat[valueIndex + 1] == QLatin1Char('c')) {
                        perm->setCurrentValue(QStringLiteral("create"));
                    }
                }
            }
        }
    }

    const KConfigGroup sessionBusGroup = parser.group("Session Bus Policy");
    if(sessionBusGroup.exists()) {
        QMap<QString, QString> busMap = sessionBusGroup.entryMap();
        QList<QString> busList = busMap.keys();
        QStringList addedBusList;
        while(i < m_permissions.length()) {
            FlatpakPermission *perm = &m_permissions[i];
            if(perm->category() != QStringLiteral("Session Bus Policy")) {
                break;
            }
            addedBusList.append(perm->name());
            int index = busList.indexOf(perm->name());
            if(index != -1) {
                QString value = busMap.value(busList.at(index));
                perm->setCurrentValue(value);
            }
            i++;
        }
        for(int j = 0; j < busList.length(); ++j) {
            if(!addedBusList.contains(busList.at(j))) {
                QString name = busList.at(j);
                QString description = name;
                QString value = busMap.value(busList.at(j));
                QString category = QStringLiteral("Session Bus Policy");
                QStringList possibleValues;
                possibleValues << QStringLiteral("talk") << QStringLiteral("see") << QStringLiteral("own");
                if(i != m_permissions.length())
                m_permissions.insert(i, FlatpakPermission(name, category, description, QStringLiteral("OFF"), possibleValues, QStringLiteral("OFF"), FlatpakPermission::ValueType::Complex));
                else
                    m_permissions.append(FlatpakPermission(name, category, description, QStringLiteral("OFF"), possibleValues, QStringLiteral("OFF"), FlatpakPermission::ValueType::Complex));
                m_permissions[i].setCurrentValue(value);
                i++;
            }
        }
    }

    const KConfigGroup systemBusGroup = parser.group("System Bus Policy");
    if(systemBusGroup.exists()) {
        QMap<QString, QString> busMap = systemBusGroup.entryMap();
        QList<QString> busList = busMap.keys();
        QStringList addedBusList;
        while(i < m_permissions.length()) {
            FlatpakPermission *perm = &m_permissions[i];
            if(perm->category() != QStringLiteral("System Bus Policy")) {
                break;
            }
            addedBusList.append(perm->name());
            int index = busList.indexOf(perm->name());
            if(index != -1) {
                QString value = busMap.value(busList.at(index));
                perm->setCurrentValue(value);
            }
            i++;
        }
        for(int j = 0; j < busList.length(); ++j) {
            if(!addedBusList.contains(busList.at(j))) {
                QString name = busList.at(j);
                QString description = name;
                QString value = busMap.value(busList.at(j));
                QString category = QStringLiteral("System Bus Policy");
                QStringList possibleValues;
                possibleValues << QStringLiteral("talk") << QStringLiteral("see") << QStringLiteral("own");
                if(i != m_permissions.length()) {
                    m_permissions.insert(i, FlatpakPermission(name, category, description, QStringLiteral("OFF"), possibleValues, QStringLiteral("OFF"), FlatpakPermission::ValueType::Complex));
                } else {
                    m_permissions.append(FlatpakPermission(name, category, description, QStringLiteral("OFF"), possibleValues, QStringLiteral("OFF"), FlatpakPermission::ValueType::Complex));
                }
                m_permissions[i].setCurrentValue(value);
                i++;
            }
        }
    }
}

FlatpakReference *FlatpakPermissionModel::reference()
{
    return m_reference;
}

void FlatpakPermissionModel::setReference(FlatpakReference *ref)
{
    if(m_reference != ref) {
        beginResetModel();
        m_reference = ref;
        m_permissions.clear();
        loadDefaultValues();
        endResetModel();
        Q_EMIT referenceChanged();
    }
}

void FlatpakPermissionModel::setPerm(int index, bool isGranted)
{
    FlatpakPermission *perm = &m_permissions[index];
    QFile inFile(m_reference->path());

    if(!inFile.open(QIODevice::ReadOnly)) {
        qInfo() << "File does not open for reading";
    }

    QTextStream inStream(&inFile);
    QString data = inStream.readAll();
    inFile.close();

    if(perm->type() == FlatpakPermission::ValueType::Simple || perm->type() == FlatpakPermission::ValueType::Complex) {

        /* For simple permissions:
         * if default is ON, and user turns it OFF, write "category=!perm" in file.
         * if default is ON, and user turns it ON from OFF, remove "category=!perm" from file.
         * if default is OFF, and user turns it ON, write "category=perm" in file.
         * if default is OFF, and user turns it OFF from ON, remove "category=perm" from file.
         */

        bool setToNonDefault = (perm->defaultValue() == QStringLiteral("ON") && isGranted) || (perm->defaultValue() == QStringLiteral("OFF") && !isGranted);

        /* set the permission from default to a non-default value */
        if(setToNonDefault) {
            addPermission(perm, data, !isGranted);

        /* reset the permission to default from non-default value */
        } else {
            removePermission(perm, data, !isGranted);
        }

        /* set the current value in the permission object */
        QString newValue = isGranted ? QStringLiteral("OFF") : QStringLiteral("ON");
        perm->setCurrentValue(newValue);
    } else if (perm->type() == FlatpakPermission::ValueType::Filesystems) {
        if (perm->defaultValue() != QStringLiteral("OFF") && isGranted) {
            qInfo() << perm->currentValue();
            removePermission(perm, data, isGranted);
            qInfo() << perm->currentValue();
            addPermission(perm, data, !isGranted);
        } else if (perm->defaultValue() == QStringLiteral("OFF") && !isGranted) {
            addPermission(perm, data, !isGranted);
            perm->setCurrentValue(perm->defaultValue());
        } else if (perm->defaultValue() != QStringLiteral("OFF") && !isGranted) {
            qInfo() << perm->currentValue();
            removePermission(perm, data, isGranted);
            qInfo() << perm->currentValue();
        } else if (perm->defaultValue() == QStringLiteral("OFF") && isGranted) {
            removePermission(perm, data, isGranted);
        }
    }

    QFile outFile(m_reference->path());
    if(!outFile.open(QIODevice::WriteOnly)) {
        qInfo() << "File does not open for write only";
    }

    QTextStream outStream(&outFile);
    outStream << data;
    outFile.close();
}

void FlatpakPermissionModel::editPerm(int index, QString newValue)
{
    /* reading from file */
    FlatpakPermission *perm = &m_permissions[index];
    QFile inFile(m_reference->path());

    if(!inFile.open(QIODevice::ReadOnly)) {
        qInfo() << "File does not open for reading";
    }

    QTextStream inStream(&inFile);
    QString data = inStream.readAll();
    inFile.close();

    /* editing the permission */
    if(perm->category() == QStringLiteral("filesystems")) {
        editFilesystemsPermissions(perm, data, newValue);
    } else if(perm->category() == QStringLiteral("Session Bus Policy") || perm->category() == QStringLiteral("System Bus Policy")) {
        editBusPermissions(perm, data, newValue);
    }

    /* writing to file */
    QFile outFile(m_reference->path());
    if(!outFile.open(QIODevice::WriteOnly)) {
        qInfo() << "File does not open for write only";
    }

    QTextStream outStream(&outFile);
    outStream << data;
    outFile.close();
}

void FlatpakPermissionModel::addPermission(FlatpakPermission *perm, QString &data, const bool shouldBeOn)
{
    if(!data.contains(QStringLiteral("[Context]"))) {
        data.insert(data.length(), QStringLiteral("[Context]\n"));
    }
    int catIndex = data.indexOf(perm->category());

    /* if no permission from this category has been changed from default, we need to add the category */
    if(catIndex == -1) {
        catIndex = data.indexOf(QLatin1Char('\n'), data.indexOf(QStringLiteral("[Context]"))) + 1;
        if(catIndex == data.length()) {
            data.append(perm->category() + QStringLiteral("=\n"));
        } else {
            data.insert(catIndex, perm->category() + QStringLiteral("=\n"));
        }
    }
    QString name = perm->name(); /* the name of the permission we are about to set/unset */
    if (perm->type() == FlatpakPermission::ValueType::Filesystems) {
        name.append(QLatin1Char(':') + perm->fsCurrentValue());
    }
    int permIndex = catIndex + perm->category().length() + 1;

    /* if there are other permissions in this category, we must add a ';' to seperate this from the other */
    if(data[permIndex] != QLatin1Char('\n')) {
        name.append(QLatin1Char(';'));
    }
    /* if permission was granted before user clicked to change it, we must un-grant it. And vice-versa */
    if(!shouldBeOn) {
        name.prepend(QLatin1Char('!'));
    }

    if(permIndex >= data.length()) {
        data.append(name);
    } else {
        data.insert(permIndex, name);
    }
}

void FlatpakPermissionModel::removePermission(FlatpakPermission *perm, QString &data, const bool isGranted)
{
    int permStartIndex = data.indexOf(perm->name());
    int permEndIndex = permStartIndex + perm->name().length();

    if (permStartIndex == -1) {
        return;
    }

    /* if it is not granted, there exists a ! one index before the name that should also be deleted */
    if(!isGranted) {
        permStartIndex--;
    }

    if (perm->type() == FlatpakPermission::ValueType::Filesystems) {
        if (data[permEndIndex] == QLatin1Char(':')) {
            permEndIndex += perm->fsCurrentValue().length() + 1;
            if (isGranted) {
                perm->setCurrentValue(QStringLiteral("OFF"));
            } else {
                perm->setCurrentValue(perm->defaultValue());
            }
        }
    }

    /* if last permission in the list, include ';' of 2nd last too */
    if(data[permEndIndex] != QLatin1Char(';')) {
        permEndIndex--;
        if(data[permStartIndex - 1] == QLatin1Char(';')) {
            permStartIndex--;
        }
    }
    data.remove(permStartIndex, permEndIndex - permStartIndex + 1);

    /* remove category entry if there are no more permission entries */
    int catIndex = data.indexOf(perm->category());
    if(data[data.indexOf(QLatin1Char('='), catIndex) + 1] == QLatin1Char('\n')) {
        data.remove(catIndex, perm->category().length() + 2); // 2 because we need to remove '\n' as well
    }
}

void FlatpakPermissionModel::addBusPermissions(FlatpakPermission *perm, QString &data)
{
    QString groupHeader = QLatin1Char('[') + perm->category() + QLatin1Char(']');
    if(!data.contains(groupHeader)) {
        data.insert(data.length(), groupHeader + QLatin1Char('\n'));
    }
    int permIndex = data.indexOf(QLatin1Char('\n'), data.indexOf(groupHeader)) + 1;
    data.insert(permIndex, perm->name() + QLatin1Char('=') + perm->currentValue() + QLatin1Char('\n'));
}

void FlatpakPermissionModel::editFilesystemsPermissions(FlatpakPermission *perm, QString &data, const QString &newValue)
{
    QString value;
    if(newValue == QStringLiteral("read-only")){
        value = QStringLiteral(":ro");
    } else if(newValue == QStringLiteral("create")) {
        value = QStringLiteral(":create");
    } else {
        value = QStringLiteral(":rw");
    }

    int permIndex = data.indexOf(perm->name());
    if(permIndex == -1) {
        addPermission(perm, data, true);
        permIndex = data.indexOf(perm->name());
    }
    int valueBeginIndex = permIndex + perm->name().length();
    if(data[valueBeginIndex] != QLatin1Char(':')) {
        data.insert(permIndex + perm->name().length(), value);
    } else {
        data.insert(valueBeginIndex, value);
        if(data[valueBeginIndex + value.length() + 1] ==  QLatin1Char('r')) {
            data.remove(valueBeginIndex + value.length(), 3);
        } else {
            data.remove(valueBeginIndex + value.length(), 7);
        }
    }
    perm->setCurrentValue(newValue);
}

void FlatpakPermissionModel::editBusPermissions(FlatpakPermission *perm, QString &data, const QString &value)
{
    int permIndex = data.indexOf(perm->name());
    if(permIndex == -1) {
        addBusPermissions(perm, data);
        permIndex = data.indexOf(perm->name());
    }
    int valueBeginIndex = permIndex + perm->name().length();
    data.insert(valueBeginIndex, QLatin1Char('=') + value);
    if(data[valueBeginIndex + value.length() + 2] ==  QLatin1Char('t')) {
        data.remove(valueBeginIndex + value.length() + 1, 5);
    } else {
        data.remove(valueBeginIndex + value.length() + 1, 4);
    }

    perm->setCurrentValue(value);
}
