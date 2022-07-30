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
      m_type(type),
      m_defaultValue(defaultValue),
      m_possibleValues(possibleValues),
      m_currentValue(currentValue)
{
}

FlatpakPermission::FlatpakPermission(QString name, QString category, QString description, ValueType type, bool isEnabledByDefault, QString defaultValue, QStringList possibleValues)
    : m_name(name),
      m_category(category),
      m_description(description),
      m_type(type),
      m_isEnabledByDefault(isEnabledByDefault),
      m_defaultValue(defaultValue),
      m_possibleValues(possibleValues)
{
    /* place-holders, will override while loading current values */
    m_isEnabled = isEnabledByDefault;
    m_currentValue = m_defaultValue;
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
    if (m_currentValue == i18n("OFF")) {
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

QString FlatpakPermission::category() const
{
    return m_category;
}

QString FlatpakPermission::categoryHeading() const
{
    if (m_category == QStringLiteral("shared")) {
        return i18n("Subsystems Shared");
    } else if (m_category == QStringLiteral ("sockets")) {
        return i18n("Sockets");
    } else if (m_category == QStringLiteral("devices")) {
        return i18n("Device Access");
    } else if (m_category == QStringLiteral("features")) {
        return i18n("Features Allowed");
    } else if (m_category == QStringLiteral("filesystems")) {
        return i18n("Filesystem Access");
    } else {
        return m_category;
    }
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

FlatpakPermission::PermType FlatpakPermission::pType() const
{
    return m_pType;
}

bool FlatpakPermission::enabled() const
{
    return m_isEnabled;
}

bool FlatpakPermission::enabledByDefault() const
{
    return m_isEnabledByDefault;
}

void FlatpakPermission::setCurrentValue(QString val)
{
    m_currentValue = val;
}

void FlatpakPermission::setEnabled(bool isEnabled)
{
    m_isEnabled = isEnabled;
}

void FlatpakPermission::setPType(PermType pType)
{
    m_pType = pType;
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
        return m_permissions.at(index.row()).categoryHeading();
    case Roles::Description:
        return m_permissions.at(index.row()).description();
    case Roles::CurrentValue:
        return m_permissions.at(index.row()).currentValue();
    case Roles::DefaultValue:
        return m_permissions.at(index.row()).defaultValue();
    case Roles::IsGranted:
        return m_permissions.at(index.row()).enabled();
    case Roles::Type:
        return m_permissions.at(index.row()).type();
    case Roles::IsSimple:
        return m_permissions.at(index.row()).type() == FlatpakPermission::ValueType::Simple;
    case Roles::IsEnvironment:
        return m_permissions.at(index.row()).type() == FlatpakPermission::Environment;
    case Roles::ValueList:
        QStringList valuesTmp = m_permissions.at(index.row()).possibleValues();
        QString currentVal = m_permissions.at(index.row()).currentValue();
        valuesTmp.removeAll(currentVal);
        valuesTmp.prepend(currentVal);
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
    roles[Roles::IsEnvironment] = "isEnvironment";
    return roles;
}

void FlatpakPermissionModel::loadDefaultValues()
{
    QString name, category, description, defaultValue;
    QStringList possibleValues;
    bool isEnabledByDefault;

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

    name = i18n("network");
    description = i18n("Internet Connection");
    isEnabledByDefault = sharedPerms.contains(name);
    m_permissions.append(FlatpakPermission(name, category, description, FlatpakPermission::Simple, isEnabledByDefault));

    name = i18n("ipc");
    description = i18n("Inter-process Communication");
    isEnabledByDefault = sharedPerms.contains(name);
    m_permissions.append(FlatpakPermission(name, category, description, FlatpakPermission::Simple, isEnabledByDefault));
    /* SHARED category */

    /* SOCKETS category */
    category = i18n("sockets");
    const QString socketPerms = contextGroup.readEntry("sockets", QString());

    name = i18n("x11");
    description = i18n("X11 Windowing System");
    isEnabledByDefault = socketPerms.contains(name);
    m_permissions.append(FlatpakPermission(name, category, description, FlatpakPermission::Simple, isEnabledByDefault));

    name = i18n("wayland");
    description = i18n("Wayland Windowing System");
    isEnabledByDefault = socketPerms.contains(name);
    m_permissions.append(FlatpakPermission(name, category, description, FlatpakPermission::Simple, isEnabledByDefault));

    name = i18n("fallback-x11");
    description = i18n("Fallback to X11 Windowing System");
    isEnabledByDefault = socketPerms.contains(name);
    m_permissions.append(FlatpakPermission(name, category, description, FlatpakPermission::Simple, isEnabledByDefault));

    name = i18n("pulseaudio");
    description = i18n("Pulseaudio Sound Server");
    isEnabledByDefault = socketPerms.contains(name);
    m_permissions.append(FlatpakPermission(name, category, description, FlatpakPermission::Simple, isEnabledByDefault));

    name = i18n("session-bus");
    description = i18n("Session Bus Access");
    isEnabledByDefault = socketPerms.contains(name);
    m_permissions.append(FlatpakPermission(name, category, description, FlatpakPermission::Simple, isEnabledByDefault));

    name = i18n("system-bus");
    description = i18n("System Bus Access");
    isEnabledByDefault = socketPerms.contains(name);
    m_permissions.append(FlatpakPermission(name, category, description, FlatpakPermission::Simple, isEnabledByDefault));

    name = i18n("ssh-auth");
    description = i18n("Remote Login Access");
    isEnabledByDefault = socketPerms.contains(name);
    m_permissions.append(FlatpakPermission(name, category, description, FlatpakPermission::Simple, isEnabledByDefault));

    name = i18n("pcsc");
    description = i18n("Smart Card Access");
    isEnabledByDefault = socketPerms.contains(name);
    m_permissions.append(FlatpakPermission(name, category, description, FlatpakPermission::Simple, isEnabledByDefault));

    name = i18n("cups");
    description = i18n("Print System Access");
    isEnabledByDefault = socketPerms.contains(name);
    m_permissions.append(FlatpakPermission(name, category, description, FlatpakPermission::Simple, isEnabledByDefault));
    /* SOCKETS category */

    /* DEVICES category */
    category = i18n("devices");
    const QString devicesPerms = contextGroup.readEntry("devices", QString());

    name = i18n("kvm");
    description = i18n("Kernel-based Virtual Machine Access");
    isEnabledByDefault = devicesPerms.contains(name);
    m_permissions.append(FlatpakPermission(name, category, description, FlatpakPermission::Simple, isEnabledByDefault));

    name = i18n("dri");
    description = i18n("Direct Graphic Rendering");
    isEnabledByDefault = devicesPerms.contains(name);
    m_permissions.append(FlatpakPermission(name, category, description, FlatpakPermission::Simple, isEnabledByDefault));

    name = i18n("shm");
    description = i18n("Host dev/shm");
    isEnabledByDefault = devicesPerms.contains(name);
    m_permissions.append(FlatpakPermission(name, category, description, FlatpakPermission::Simple, isEnabledByDefault));

    name = i18n("all");
    description = i18n("Device Access");
    isEnabledByDefault = devicesPerms.contains(name);
    m_permissions.append(FlatpakPermission(name, category, description, FlatpakPermission::Simple, isEnabledByDefault));
    /* DEVICES category */

    /* FEATURES category */
    category = i18n("features");
    const QString featuresPerms = contextGroup.readEntry("features", QString());

    name = i18n("devel");
    description = i18n("System Calls by Development Tools");
    isEnabledByDefault = featuresPerms.contains(name);
    m_permissions.append(FlatpakPermission(name, category, description, FlatpakPermission::Simple, isEnabledByDefault));

    name = i18n("multiarch");
    description = i18n("Run Multiarch/Multilib Binaries");
    isEnabledByDefault = featuresPerms.contains(name);
    m_permissions.append(FlatpakPermission(name, category, description, FlatpakPermission::Simple, isEnabledByDefault));

    name = i18n("bluetooth");
    description = i18n("Bluetooth");
    isEnabledByDefault = featuresPerms.contains(name);
    m_permissions.append(FlatpakPermission(name, category, description, FlatpakPermission::Simple, isEnabledByDefault));

    name = i18n("canbus");
    description = i18n("Canbus Socket Access");
    isEnabledByDefault = featuresPerms.contains(name);
    m_permissions.append(FlatpakPermission(name, category, description, FlatpakPermission::Simple, isEnabledByDefault));

    name = i18n("per-app-dev-shm");
    description = i18n("Share dev/shm across all instances of an app per user ID");
    isEnabledByDefault = featuresPerms.contains(name);
    m_permissions.append(FlatpakPermission(name, category, description, FlatpakPermission::Simple, isEnabledByDefault));
    /* FEATURES category */

    /* FILESYSTEM category */
    category = i18n("filesystems");
    const QString fileSystemPerms = contextGroup.readEntry("filesystems", QString());
    const auto dirs = QStringView(fileSystemPerms).split(QLatin1Char(';'), Qt::SkipEmptyParts);

    QString homeVal, hostVal, hostOsVal, hostEtcVal;
    homeVal = hostVal = hostOsVal = hostEtcVal = i18n("OFF");
    possibleValues.clear();
    possibleValues << i18n("read/write") << i18n("read-only") << i18n("create");

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
            isEnabledByDefault = true;
            filesysTemp.append(FlatpakPermission(name, category, description, FlatpakPermission::Filesystems, isEnabledByDefault, fsval, possibleValues));
        }
    }

    name = i18n("home");
    description = i18n("Home Folder");
    possibleValues.removeAll(homeVal);
    if (homeVal == i18n("OFF")) {
        isEnabledByDefault = false;
        homeVal = i18n("read/write");
    } else {
        isEnabledByDefault = true;
    }
    m_permissions.append(FlatpakPermission(name, category, description, FlatpakPermission::Filesystems, isEnabledByDefault, homeVal, possibleValues));

    name = i18n("host");
    description = i18n("All System Files");
    if (hostVal == i18n("OFF")) {
        isEnabledByDefault = false;
        hostVal = i18n("read/write");
    } else {
        isEnabledByDefault = true;
    }
    m_permissions.append(FlatpakPermission(name, category, description, FlatpakPermission::Filesystems, isEnabledByDefault, hostVal, possibleValues));

    name = i18n("host-os");
    description = i18n("System Libraries, Executables and Binaries");
    if (hostOsVal == i18n("OFF")) {
        isEnabledByDefault = false;
        hostOsVal = i18n("read/write");
    } else {
        isEnabledByDefault = true;
    }
    m_permissions.append(FlatpakPermission(name, category, description, FlatpakPermission::Filesystems, isEnabledByDefault, hostOsVal, possibleValues));

    name = i18n("host-etc");
    description = i18n("System Configurations");
    if (hostEtcVal == i18n("OFF")) {
        isEnabledByDefault = false;
        hostEtcVal = i18n("read/write");
    } else {
        isEnabledByDefault = true;
    }
    m_permissions.append(FlatpakPermission(name, category, description, FlatpakPermission::Filesystems, isEnabledByDefault, hostEtcVal, possibleValues));

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
            isEnabledByDefault = true;
//            m_permissions.append(FlatpakPermission(name, category, description, defaultValue, possibleValues, defaultValue, FlatpakPermission::ValueType::Complex));
            m_permissions.append(FlatpakPermission(name, category, description, FlatpakPermission::Bus, isEnabledByDefault, defaultValue, possibleValues));
        }
    }
    /* SESSION BUS category */

    /* SYSTEM BUS category */
    category = i18n("System Bus Policy");
    const KConfigGroup systemBusGroup = parser.group("System Bus Policy");
    possibleValues.clear();
    possibleValues << i18n("talk") << i18n("own") << i18n("see");
    if(systemBusGroup.exists()) {
        const QMap<QString, QString> busMap = systemBusGroup.entryMap();
        const QStringList busList = busMap.keys();
        for(int i = 0; i < busList.length(); ++i) {
            name = description = busList.at(i);
            defaultValue = busMap.value(busList.at(i));
            isEnabledByDefault = true;
            m_permissions.append(FlatpakPermission(name, category, description, FlatpakPermission::Bus, isEnabledByDefault, defaultValue, possibleValues));
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
            //m_permissions.append(FlatpakPermission(name, category, description, defaultValue, possibleValues, defaultValue, FlatpakPermission::ValueType::Complex));
            m_permissions.append(FlatpakPermission(name, category, description, FlatpakPermission::Environment, true, defaultValue));
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

    int fsIndex = -1, sessionBusIndex = -1, systemBusIndex = -1, envIndex = -1;

    int i;
    for(i = 0; i < m_permissions.length(); ++i) {
        FlatpakPermission *perm = &m_permissions[i];

        if(perm->type() == FlatpakPermission::Simple) {
            const QString cat = contextGroup.readEntry(perm->category(), QString());
            if(cat.contains(perm->name())) {
                perm->setEnabled(!perm->enabledByDefault());
            }
        } else if (perm->type() == FlatpakPermission::Filesystems){
            const QString cat = contextGroup.readEntry(perm->category(), QString());
            if(cat.contains(perm->name())) {
                int permIndex = cat.indexOf(perm->name());
                /* the permission is just being set off, the access level isn't changed */
                if(permIndex > 0 && cat[permIndex - 1] == QLatin1Char('!')) {
                    perm->setEnabled(false);
                } else {
                    perm->setEnabled(true);
                }

                int valueIndex = cat.indexOf(QLatin1Char(':'), cat.indexOf(perm->name()));

                if(valueIndex == -1) {
                    perm->setCurrentValue(i18n("read/write"));
                    continue;
                }

                if(cat[valueIndex + 1] == QLatin1Char('r')) {
                    if(cat[valueIndex + 2] == QLatin1Char('w')) {
                        perm->setCurrentValue(i18n("read/write"));
                    } else {
                        perm->setCurrentValue(i18n("read-only"));
                    }
                } else if(cat[valueIndex + 1] == QLatin1Char('c')) {
                    perm->setCurrentValue(i18n("create"));
                }

            }
            fsIndex = i;
        } else if (perm->type() == FlatpakPermission::Bus || perm->type() == FlatpakPermission::Environment) {
            const KConfigGroup group = parser.group(perm->category());
            if (group.exists()) {
                QMap <QString, QString> map = group.entryMap();
                QList <QString> list = map.keys();
                int index = list.indexOf(perm->name());
                if (index != -1) {
                    bool enabled = true;
                    QString value = map.value(list.at(index));
                    QString offSig = perm->type() == FlatpakPermission::Bus ? QStringLiteral("None") : QString();
                    if (value == offSig) {
                        enabled = false;
                        value = perm->currentValue();
                    }
                    perm->setCurrentValue(value);
                    perm->setEnabled(enabled);
                }
            }
            if (perm->category() == QStringLiteral("Session Bus Policy")) {
                sessionBusIndex = i;
            } else if (perm->category() == QStringLiteral("System Bus Policy")) {
                systemBusIndex = i;
            } else {
                envIndex = i;
            }
        }
    }
    int additionalPerms = 0;
    const QString fsCat = contextGroup.readEntry(QStringLiteral("filesystems"), QString());
    if (!fsCat.isEmpty()) {
        const QStringList fsPerms = fsCat.split(QLatin1Char(';'));
        for (int j = 0; j < fsPerms.length(); ++j) {
            QString name = fsPerms.at(j);
            QString value;
            int len;
            bool enabled;
            int valBeginIndex = name.indexOf(QLatin1Char(':'));
            if (valBeginIndex != -1) {
                if (name[valBeginIndex + 1] == QLatin1Char('r')) {
                    len = 3;
                    if (name[valBeginIndex + 2] == QLatin1Char('o')) {
                        value = i18n("read-only");
                    } else {
                        value = i18n("read/write");
                    }
                } else {
                    len = 7;
                    value = i18n("create");
                }
                name.remove(valBeginIndex, len);
                enabled = name[0] != QLatin1Char('!');
                if (!enabled) {
                    name.remove(0, 1);
                }
            }
            if (!permExists(name)) {
                //fsIndex++;
                QStringList possibleValues;
                possibleValues << i18n("read/write") << i18n("read-only") << i18n("create");
                m_permissions.insert(fsIndex, FlatpakPermission(name, QStringLiteral("filesystems"), name, FlatpakPermission::Filesystems, false, value, possibleValues));
                m_permissions[fsIndex].setEnabled(enabled);
                fsIndex++;
                additionalPerms++;
            }
        }
    }

    QVector<QString> cats = {QStringLiteral("Session Bus Policy"), QStringLiteral("System Bus Policy"), QStringLiteral("Environment")};
    for (int j = 0; j < cats.length(); j++) {
        const KConfigGroup group = parser.group(cats.at(j));
        if (!group.exists()) {
            continue;
        }
        bool flag = true;
        int *insertIndex;
        if (j == 0) {
            insertIndex = &sessionBusIndex;
        } else if (j == 1) {
            insertIndex = &systemBusIndex;
        } else {
            insertIndex = &envIndex;
        }

        if (*insertIndex == -1) {
            *insertIndex = m_permissions.length();
            flag = false;
        } else {
            *insertIndex += additionalPerms;
        }

        QMap <QString, QString> map = group.entryMap();
        QList <QString> list = map.keys();
        for (int k = 0; k < list.length(); k++) {
            if (!permExists(list.at(k))) {
                QString name = list.at(k);
                QString value = map.value(name);
                bool enabled = (j == 0 || j == 1) ? value != i18n("None") : !value.isEmpty();
                if (j == 0 || j == 1) {
                    QStringList possibleValues;
                    possibleValues << i18n("talk") << i18n("own") << i18n("see");
                    m_permissions.insert(*insertIndex, FlatpakPermission(name, cats[j], name, FlatpakPermission::Bus, false, value, possibleValues));
                } else {
                    m_permissions.insert(*insertIndex, FlatpakPermission(name, cats[j], name, FlatpakPermission::Environment, false, value));
                }
                m_permissions[*insertIndex].setEnabled(enabled);
                *insertIndex += 1;
                if (flag) {
                    additionalPerms++;
                }
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
    /* guard for invalid indices */
    if (index < 0 || index >= m_permissions.length()) {
        return;
    }

    FlatpakPermission *perm = &m_permissions[index];
    QFile inFile(m_reference->path());

    if(!inFile.open(QIODevice::ReadOnly)) {
        qInfo() << "File does not open for reading";
    }

    QTextStream inStream(&inFile);
    QString data = inStream.readAll();
    inFile.close();

    if (perm->type() == FlatpakPermission::Simple) {
        bool setToNonDefault = (perm->enabledByDefault() && isGranted) || (!perm->enabledByDefault() && !isGranted);
        if (setToNonDefault) {
            addPermission(perm, data, !isGranted);
        } else {
            removePermission(perm, data, isGranted);
        }
        perm->setEnabled(!perm->enabled());
    } else if (perm->type() == FlatpakPermission::Filesystems) {
        if (perm->enabledByDefault() && isGranted) {
            /* if access level ("value") was changed, there's already an entry. Prepend ! to it.
             * if access level is unchanged, add a new entry */
            if (perm->defaultValue() != perm->currentValue()) {
                int permIndex = data.indexOf(perm->name());
                data.insert(permIndex, QLatin1Char('!'));
            } else {
                addPermission(perm, data, !isGranted);
            }
        } else if (!perm->enabledByDefault() && !isGranted) {
            if (!data.contains(perm->name())) {
                addPermission(perm, data, !isGranted);
            }
            if (perm->pType() == FlatpakPermission::UserDefined) {
                data.remove(data.indexOf(perm->name()) - 1, 1);
            }
            // set value to read/write automatically, if not done already
        } else if (perm->enabledByDefault() && !isGranted) {
            /* if access level ("value") was changed, just remove ! from beginning.
             * if access level is unchanged, remove the whole entry */
            if (perm->defaultValue() != perm->currentValue()) {
                int permIndex = data.indexOf(perm->name());
                data.remove(permIndex - 1, 1);
            } else {
                removePermission(perm, data, isGranted);
            }
        } else if (!perm->enabledByDefault() && isGranted) {
            removePermission(perm, data, isGranted);
            m_permissions.remove(index, 1);
            Q_EMIT layoutChanged();
        }
        perm->setEnabled(!perm->enabled());
    } else if (perm->type() == FlatpakPermission::Bus) {
        if (perm->enabledByDefault() && isGranted) {
            perm->setEnabled(false);
            if (perm->defaultValue() != perm->currentValue()) {
                editBusPermissions(perm, data, i18n("None"));
            } else {
                addBusPermissions(perm, data);
            }
        } else if (perm->enabledByDefault() && !isGranted) {
            if (perm->defaultValue() != perm->currentValue()) {
                editBusPermissions(perm, data, perm->currentValue());
            } else {
                removeBusPermission(perm, data);
            }
            perm->setEnabled(true);
        } else if (!perm->enabledByDefault() && isGranted) {
            removeBusPermission(perm, data);
            m_permissions.remove(index, 1);
            Q_EMIT layoutChanged();
        }
    } else if (perm->type() == FlatpakPermission::Environment) {
        if (perm->enabledByDefault() && isGranted) {
            perm->setEnabled(false);
            if (perm->defaultValue() != perm->currentValue()) {
                editEnvPermission(perm, data, QString());
            } else {
                addEnvPermission(perm, data);
            }
        } else if (perm->enabledByDefault() && !isGranted) {
            if (perm->defaultValue() != perm->currentValue()) {
                editEnvPermission(perm, data, perm->currentValue());
            } else {
                removeEnvPermission(perm, data);
            }
            perm->setEnabled(true);
        } else if (!perm->enabledByDefault() && isGranted) {
            removeEnvPermission(perm, data);
            m_permissions.remove(index, 1);
            Q_EMIT layoutChanged();
        }
    }

    QFile outFile(m_reference->path());
    if(!outFile.open(QIODevice::WriteOnly)) {
        qInfo() << "File does not open for write only";
    }

    QTextStream outStream(&outFile);
    outStream << data;
    outFile.close();

    Q_EMIT dataChanged(FlatpakPermissionModel::index(index, 0), FlatpakPermissionModel::index(index, 0));
}

void FlatpakPermissionModel::editPerm(int index, QString newValue)
{
    /* guard for out-of-range indices */
    if (index < 0 || index > m_permissions.length()) {
        return;
    }

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
    } else if (perm->type() == FlatpakPermission::Bus) {
        editBusPermissions(perm, data, newValue);
    } else if (perm->type() == FlatpakPermission::Environment) {
        editEnvPermission(perm, data, newValue);
    }

    Q_EMIT dataChanged(FlatpakPermissionModel::index(index, 0), FlatpakPermissionModel::index(index, 0));

    /* writing to file */
    QFile outFile(m_reference->path());
    if(!outFile.open(QIODevice::WriteOnly)) {
        qInfo() << "File does not open for write only";
    }

    QTextStream outStream(&outFile);
    outStream << data;
    outFile.close();
}

void FlatpakPermissionModel::addUserEnteredPermission(QString name, QString cat)
{
    QString value;
    FlatpakPermission::ValueType type;
    if (cat == QStringLiteral("filesystems")) {
        type = FlatpakPermission::Filesystems;
        value = i18n("read/write");
    } else if (cat == QStringLiteral("Session Bus Policy") || cat == QStringLiteral("System Bus Policy")) {
        type = FlatpakPermission::Bus;
        value = i18n("talk");
    } else {
        type = FlatpakPermission::Environment;
    }

    FlatpakPermission perm(name, cat, name, type, false);
    perm.setPType(FlatpakPermission::UserDefined);
    perm.setEnabled(true);
    perm.setCurrentValue(value);
    int i;

    /* first, go to the index where the permissions of this category begin. then, go to the index where they end */
    for (i = 0; i < m_permissions.length() && m_permissions.at(i).category() != cat; i++);
    if (i != m_permissions.length()) {
        for (; i < m_permissions.length() && m_permissions.at(i).category() == cat; i++);
    }

    if (i < m_permissions.length()) {
        m_permissions.insert(i, perm);
    } else {
        m_permissions.append(perm);
    }
    m_permissions[i].setEnabled(true);

    QFile inFile(m_reference->path());

    if(!inFile.open(QIODevice::ReadOnly)) {
        qInfo() << "File does not open for reading";
    }

    QTextStream inStream(&inFile);
    QString data = inStream.readAll();
    inFile.close();

    if (type == FlatpakPermission::Filesystems) {
        addPermission(&perm, data, true);
    } else if (type == FlatpakPermission::Bus) {
        addBusPermissions(&perm, data);
    } else {
        addEnvPermission(&perm, data);
    }

    QFile outFile(m_reference->path());
    if(!outFile.open(QIODevice::WriteOnly)) {
        qInfo() << "File does not open for write only";
    }

    QTextStream outStream(&outFile);
    outStream << data;
    outFile.close();

    Q_EMIT layoutChanged();
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
            data.append(perm->category() + i18n("=\n"));
        } else {
            data.insert(catIndex, perm->category() + i18n("=\n"));
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
    QString val = perm->enabled() ? perm->currentValue() : i18n("None");
    data.insert(permIndex, perm->name() + QLatin1Char('=') + val + QLatin1Char('\n'));
}

void FlatpakPermissionModel::removeBusPermission(FlatpakPermission *perm, QString &data)
{
    int permBeginIndex = data.indexOf(perm->name());
    if (permBeginIndex == -1) {
        return;
    }

    /* if it is enabled, the current value is not None. So get the length. Otherwise, it is 4 for None */
    int valueLen = perm->enabled() ? perm->currentValue().length() : 4;

    int permLen = perm->name().length() + 1 + valueLen + 1;
    data.remove(permBeginIndex, permLen);
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

    if (perm->enabledByDefault() == perm->enabled() && perm->defaultValue() == newValue) {
        removePermission(perm, data, true);
        perm->setCurrentValue(newValue);
        return;
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
    if (perm->enabledByDefault() && value == perm->defaultValue()) {
        removeBusPermission(perm, data);
        return;
    }

    int permIndex = data.indexOf(perm->name());
    if(permIndex == -1) {
        addBusPermissions(perm, data);
        permIndex = data.indexOf(perm->name());
    }
    int valueBeginIndex = permIndex + perm->name().length();
    data.insert(valueBeginIndex, QLatin1Char('=') + value);
    if(data[valueBeginIndex + value.length() + 2] ==  QLatin1Char('t') || data[valueBeginIndex + value.length() + 2] ==  QLatin1Char('N')) {
        data.remove(valueBeginIndex + value.length() + 1, 5);
    } else {
        data.remove(valueBeginIndex + value.length() + 1, 4);
    }
    if (value != i18n("None")) {
        perm->setCurrentValue(value);
    }
}

void FlatpakPermissionModel::addEnvPermission(FlatpakPermission *perm, QString &data)
{
    QString groupHeader = QLatin1Char('[') + perm->category() + QLatin1Char(']');
    if(!data.contains(groupHeader)) {
        data.insert(data.length(), groupHeader + QLatin1Char('\n'));
    }
    int permIndex = data.indexOf(QLatin1Char('\n'), data.indexOf(groupHeader)) + 1;
    QString val = perm->enabled() ? perm->currentValue() : QString();
    data.insert(permIndex, perm->name() + QLatin1Char('=') + val + QLatin1Char('\n'));
}

void FlatpakPermissionModel::removeEnvPermission(FlatpakPermission *perm, QString &data)
{
    int permBeginIndex = data.indexOf(perm->name());
    if (permBeginIndex == -1) {
        return;
    }

    int valueLen = perm->enabled() ? perm->currentValue().length() + 1 : 0;

    int permLen = perm->name().length() + 1 + valueLen;
    data.remove(permBeginIndex, permLen);
}

void FlatpakPermissionModel::editEnvPermission(FlatpakPermission *perm, QString &data, const QString &value)
{
    if (perm->enabledByDefault() && value == perm->defaultValue()) {
        removeEnvPermission(perm, data);
        return;
    }

    int permIndex = data.indexOf(perm->name());
    if(permIndex == -1) {
        addEnvPermission(perm, data);
        permIndex = data.indexOf(perm->name());
    }
    int valueBeginIndex = permIndex + perm->name().length();
    data.insert(valueBeginIndex, QLatin1Char('=') + value);

    int oldValBeginIndex = valueBeginIndex + value.length() + 1;
    int oldValEndIndex = data.indexOf(QLatin1Char('\n'), oldValBeginIndex);
    data.remove(oldValBeginIndex, oldValEndIndex - oldValBeginIndex);

    if (!value.isEmpty()) {
        perm->setCurrentValue(value);
    }
}

bool FlatpakPermissionModel::permExists(QString name)
{
    for (int i = 0; i < m_permissions.length(); ++i) {
        if (m_permissions.at(i).name() == name) {
            return true;
        }
    }
    return false;
}
