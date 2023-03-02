/**
 * SPDX-FileCopyrightText: 2022 Suhaas Joshi <joshiesuhaas0@gmail.com>
 * SPDX-FileCopyrightText: 2023 Harald Sitter <sitter@kde.org>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "flatpakpermission.h"

#include <KConfigGroup>
#include <KDesktopFile>
#include <KLocalizedString>
#include <QDebug>
#include <QFileInfo>
#include <QTemporaryFile>

FlatpakPermission::FlatpakPermission(const QString &name,
                                     const QString &category,
                                     const QString &description,
                                     const QString &defaultValue,
                                     const QStringList &possibleValues,
                                     const QString &currentValue,
                                     ValueType type)
    : m_name(name)
    , m_category(category)
    , m_description(description)
    , m_type(type)
    , m_defaultValue(defaultValue)
    , m_possibleValues(possibleValues)
    , m_currentValue(currentValue)
{
}

FlatpakPermission::FlatpakPermission(const QString &name,
                                     const QString &category,
                                     const QString &description,
                                     ValueType type,
                                     bool isEnabledByDefault,
                                     const QString &defaultValue,
                                     const QStringList &possibleValues)
    : m_name(name)
    , m_category(category)
    , m_description(description)
    , m_type(type)
    , m_isEnabledByDefault(isEnabledByDefault)
    , m_defaultValue(defaultValue)
    , m_possibleValues(possibleValues)
{
    /* will override while loading current values */
    m_isEnabled = m_isLoadEnabled = isEnabledByDefault;
    m_currentValue = m_loadValue = m_defaultValue;
    m_sType = m_type == FlatpakPermission::Filesystems ? FlatpakPermission::Basic : FlatpakPermission::Advanced;
}

QString FlatpakPermission::name() const
{
    return m_name;
}

static QString toFrontendDBusValue(const QString &value)
{
    if (value == QStringLiteral("talk")) {
        return i18n("talk");
    }
    if (value == QStringLiteral("own")) {
        return i18n("own");
    }
    if (value == QStringLiteral("see")) {
        return i18n("see");
    }
    if (value == QStringLiteral("None")) {
        return i18n("None");
    }
    Q_ASSERT_X(false, Q_FUNC_INFO, qUtf8Printable(QStringLiteral("unmapped value %1").arg(value)));
    return {};
}

static QString toBackendDBusValue(const QString &value)
{
    if (value == i18n("talk")) {
        return QStringLiteral("talk");
    }
    if (value == i18n("own")) {
        return QStringLiteral("own");
    }
    if (value == i18n("see")) {
        return QStringLiteral("see");
    }
    if (value == i18n("None")) {
        return QStringLiteral("None");
    }
    Q_ASSERT_X(false, Q_FUNC_INFO, qUtf8Printable(QStringLiteral("unmapped value %1").arg(value)));
    return {};
}

QString FlatpakPermission::currentValue() const
{
    return m_currentValue;
}

QString FlatpakPermission::loadValue() const
{
    return m_loadValue;
}

static QString postfixToFrontendFileSystemValue(const QStringView &postfix)
{
    if (postfix == QStringLiteral(":ro")) {
        return i18n("read-only");
    }
    if (postfix == QStringLiteral(":create")) {
        return i18n("create");
    }
    return i18n("read/write");
}

QString FlatpakPermission::fsCurrentValue() const
{
    if (m_currentValue == i18n("OFF")) {
        return QString();
    }

    QString val;
    // NB: the use of i18n here is actually kind of wrong but at the time of writing fixing the mapping
    // between ui and backend *here* is easier/safer than trying to reinvent the way the mapping works in a
    // more reliable fashion.
    if (m_currentValue == i18n("read-only")) {
        val = QStringLiteral("ro");
    } else if (m_currentValue == i18n("create")) {
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
    if (m_sType == FlatpakPermission::Basic) {
        if (m_type == FlatpakPermission::Filesystems) {
            return i18n("Filesystem Access");
        }
        return i18n("Basic Permissions");
    }

    if (m_category == QStringLiteral("shared")) {
        return i18n("Subsystems Shared");
    }
    if (m_category == QStringLiteral("sockets")) {
        return i18n("Sockets");
    }
    if (m_category == QStringLiteral("devices")) {
        return i18n("Device Access");
    }
    if (m_category == QStringLiteral("features")) {
        return i18n("Features Allowed");
    }
    if (m_category == QStringLiteral("Advanced Dummy")) {
        return i18n("Advanced Permissions");
    }
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

FlatpakPermission::PermType FlatpakPermission::pType() const
{
    return m_pType;
}

FlatpakPermission::SectionType FlatpakPermission::sType() const
{
    return m_sType;
}

bool FlatpakPermission::enabled() const
{
    return m_isEnabled;
}

bool FlatpakPermission::enabledByDefault() const
{
    return m_isEnabledByDefault;
}

void FlatpakPermission::setCurrentValue(const QString &val)
{
    m_currentValue = val;
}

void FlatpakPermission::setLoadValue(const QString &loadValue)
{
    m_loadValue = loadValue;
}

void FlatpakPermission::setEnabled(bool isEnabled)
{
    m_isEnabled = isEnabled;
}

void FlatpakPermission::setLoadEnabled(bool isLoadEnabled)
{
    m_isLoadEnabled = isLoadEnabled;
}

void FlatpakPermission::setPType(PermType pType)
{
    m_pType = pType;
}

void FlatpakPermission::setSType(SectionType sType)
{
    m_sType = sType;
}

bool FlatpakPermission::isSaveNeeded() const
{
    if (m_pType == FlatpakPermission::Dummy) {
        return false;
    }

    bool ret = m_isEnabled != m_isLoadEnabled;
    if (m_type != FlatpakPermission::Simple) {
        ret = ret || (m_currentValue != m_loadValue);
    }
    return ret;
}

bool FlatpakPermission::isDefaults() const
{
    bool ret = m_isEnabled == m_isEnabledByDefault;
    if (m_type != FlatpakPermission::Simple) {
        ret = ret && (m_currentValue == m_defaultValue);
    }
    return ret;
}

int FlatpakPermissionModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return m_permissions.count();
}

QVariant FlatpakPermissionModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }

    const auto permission = m_permissions.at(index.row());

    switch (role) {
    case Roles::Name:
        return permission.name();
    case Roles::Category:
        return permission.categoryHeading();
    case Roles::Description:
        return permission.description();
    case Roles::CurrentValue:
        return permission.currentValue();
    case Roles::DefaultValue:
        return permission.defaultValue();
    case Roles::IsGranted:
        return permission.enabled();
    case Roles::Type:
        return permission.type();
    case Roles::IsSimple:
        return permission.type() == FlatpakPermission::ValueType::Simple;
    case Roles::IsEnvironment:
        return permission.type() == FlatpakPermission::Environment;
    case Roles::IsNotDummy:
        return permission.pType() != FlatpakPermission::Dummy;
    case Roles::SectionType:
        if (permission.sType() == FlatpakPermission::Basic) {
            if (permission.type() == FlatpakPermission::Filesystems) {
                return i18n("Filesystem Access");
            }
            return i18n("Basic Permissions");
        }
        return i18n("Advanced Permissions");
    case Roles::IsBasic:
        return permission.sType() == FlatpakPermission::Basic;
    case Roles::ValueList:
        QStringList valuesTmp = permission.possibleValues();
        QString currentVal = permission.currentValue();
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
    roles[Roles::IsNotDummy] = "isNotDummy";
    roles[Roles::SectionType] = "sectionType";
    roles[Roles::IsBasic] = "isBasic";
    return roles;
}

void FlatpakPermissionModel::loadDefaultValues()
{
    QString name;
    QString category;
    QString description;
    QString defaultValue;
    QStringList possibleValues;
    bool isEnabledByDefault = false;

    const QByteArray metadata = m_reference->metadata();

    QTemporaryFile f;
    if (!f.open()) {
        return;
    }
    f.write(metadata);
    f.close();

    KDesktopFile parser(f.fileName());
    const KConfigGroup contextGroup = parser.group("Context");

    int basicIndex = 0;

    /* SHARED category */
    category = QStringLiteral("shared");
    const QString sharedPerms = contextGroup.readEntry("shared", QString());

    name = QStringLiteral("network");
    description = i18n("Internet Connection");
    isEnabledByDefault = sharedPerms.contains(name);
    m_permissions.insert(basicIndex, FlatpakPermission(name, category, description, FlatpakPermission::Simple, isEnabledByDefault));
    m_permissions[basicIndex++].setSType(FlatpakPermission::Basic);

    name = QStringLiteral("ipc");
    description = i18n("Inter-process Communication");
    isEnabledByDefault = sharedPerms.contains(name);
    m_permissions.append(FlatpakPermission(name, category, description, FlatpakPermission::Simple, isEnabledByDefault));
    /* SHARED category */

    /* SOCKETS category */
    category = QStringLiteral("sockets");
    const QString socketPerms = contextGroup.readEntry("sockets", QString());

    name = QStringLiteral("x11");
    description = i18n("X11 Windowing System");
    isEnabledByDefault = socketPerms.contains(name);
    m_permissions.append(FlatpakPermission(name, category, description, FlatpakPermission::Simple, isEnabledByDefault));

    name = QStringLiteral("wayland");
    description = i18n("Wayland Windowing System");
    isEnabledByDefault = socketPerms.contains(name);
    m_permissions.append(FlatpakPermission(name, category, description, FlatpakPermission::Simple, isEnabledByDefault));

    name = QStringLiteral("fallback-x11");
    description = i18n("Fallback to X11 Windowing System");
    isEnabledByDefault = socketPerms.contains(name);
    m_permissions.append(FlatpakPermission(name, category, description, FlatpakPermission::Simple, isEnabledByDefault));

    name = QStringLiteral("pulseaudio");
    description = i18n("Pulseaudio Sound Server");
    isEnabledByDefault = socketPerms.contains(name);
    m_permissions.insert(basicIndex, FlatpakPermission(name, category, description, FlatpakPermission::Simple, isEnabledByDefault));
    m_permissions[basicIndex++].setSType(FlatpakPermission::Basic);

    name = QStringLiteral("session-bus");
    description = i18n("Session Bus Access");
    isEnabledByDefault = socketPerms.contains(name);
    m_permissions.append(FlatpakPermission(name, category, description, FlatpakPermission::Simple, isEnabledByDefault));

    name = QStringLiteral("system-bus");
    description = i18n("System Bus Access");
    isEnabledByDefault = socketPerms.contains(name);
    m_permissions.append(FlatpakPermission(name, category, description, FlatpakPermission::Simple, isEnabledByDefault));

    name = QStringLiteral("ssh-auth");
    description = i18n("Remote Login Access");
    isEnabledByDefault = socketPerms.contains(name);
    m_permissions.insert(basicIndex, FlatpakPermission(name, category, description, FlatpakPermission::Simple, isEnabledByDefault));
    m_permissions[basicIndex++].setSType(FlatpakPermission::Basic);

    name = QStringLiteral("pcsc");
    description = i18n("Smart Card Access");
    isEnabledByDefault = socketPerms.contains(name);
    m_permissions.insert(basicIndex, FlatpakPermission(name, category, description, FlatpakPermission::Simple, isEnabledByDefault));
    m_permissions[basicIndex++].setSType(FlatpakPermission::Basic);

    name = QStringLiteral("cups");
    description = i18n("Print System Access");
    isEnabledByDefault = socketPerms.contains(name);
    m_permissions.insert(basicIndex, FlatpakPermission(name, category, description, FlatpakPermission::Simple, isEnabledByDefault));
    m_permissions[basicIndex++].setSType(FlatpakPermission::Basic);
    /* SOCKETS category */

    /* DEVICES category */
    category = QStringLiteral("devices");
    const QString devicesPerms = contextGroup.readEntry("devices", QString());

    name = QStringLiteral("kvm");
    description = i18n("Kernel-based Virtual Machine Access");
    isEnabledByDefault = devicesPerms.contains(name);
    m_permissions.append(FlatpakPermission(name, category, description, FlatpakPermission::Simple, isEnabledByDefault));

    name = QStringLiteral("dri");
    description = i18n("Direct Graphic Rendering");
    isEnabledByDefault = devicesPerms.contains(name);
    m_permissions.append(FlatpakPermission(name, category, description, FlatpakPermission::Simple, isEnabledByDefault));

    name = QStringLiteral("shm");
    description = i18n("Host dev/shm");
    isEnabledByDefault = devicesPerms.contains(name);
    m_permissions.append(FlatpakPermission(name, category, description, FlatpakPermission::Simple, isEnabledByDefault));

    name = QStringLiteral("all");
    description = i18n("Device Access");
    isEnabledByDefault = devicesPerms.contains(name);
    m_permissions.insert(basicIndex, FlatpakPermission(name, category, description, FlatpakPermission::Simple, isEnabledByDefault));
    m_permissions[basicIndex++].setSType(FlatpakPermission::Basic);
    /* DEVICES category */

    /* FEATURES category */
    category = QStringLiteral("features");
    const QString featuresPerms = contextGroup.readEntry("features", QString());

    name = QStringLiteral("devel");
    description = i18n("System Calls by Development Tools");
    isEnabledByDefault = featuresPerms.contains(name);
    m_permissions.append(FlatpakPermission(name, category, description, FlatpakPermission::Simple, isEnabledByDefault));

    name = QStringLiteral("multiarch");
    description = i18n("Run Multiarch/Multilib Binaries");
    isEnabledByDefault = featuresPerms.contains(name);
    m_permissions.append(FlatpakPermission(name, category, description, FlatpakPermission::Simple, isEnabledByDefault));

    name = QStringLiteral("bluetooth");
    description = i18n("Bluetooth");
    isEnabledByDefault = featuresPerms.contains(name);
    m_permissions.insert(basicIndex, FlatpakPermission(name, category, description, FlatpakPermission::Simple, isEnabledByDefault));
    m_permissions[basicIndex++].setSType(FlatpakPermission::Basic);

    name = QStringLiteral("canbus");
    description = i18n("Canbus Socket Access");
    isEnabledByDefault = featuresPerms.contains(name);
    m_permissions.append(FlatpakPermission(name, category, description, FlatpakPermission::Simple, isEnabledByDefault));

    name = QStringLiteral("per-app-dev-shm");
    description = i18n("Share dev/shm across all instances of an app per user ID");
    isEnabledByDefault = featuresPerms.contains(name);
    m_permissions.append(FlatpakPermission(name, category, description, FlatpakPermission::Simple, isEnabledByDefault));
    /* FEATURES category */

    /* FILESYSTEM category */
    category = QStringLiteral("filesystems");
    const QString fileSystemPerms = contextGroup.readEntry("filesystems", QString());
    const auto dirs = QStringView(fileSystemPerms).split(QLatin1Char(';'), Qt::SkipEmptyParts);

    QString homeVal = i18n("OFF");
    QString hostVal = i18n("OFF");
    QString hostOsVal = i18n("OFF");
    QString hostEtcVal = i18n("OFF");
    possibleValues.clear();
    possibleValues << i18n("read/write") << i18n("read-only") << i18n("create");

    QVector<FlatpakPermission> filesysTemp;

    for (const QStringView &dir : dirs) {
        if (dir == QLatin1String("xdg-config/kdeglobals:ro")) {
            continue;
        }
        int sep = dir.lastIndexOf(QLatin1Char(':'));
        const QStringView postfix = sep > 0 ? dir.mid(sep) : QStringView();
        const QStringView symbolicName = dir.left(sep);

        if (symbolicName == QStringLiteral("home")) {
            homeVal = postfixToFrontendFileSystemValue(postfix);
        } else if (symbolicName == QStringLiteral("host")) {
            hostVal = postfixToFrontendFileSystemValue(postfix);
        } else if (symbolicName == QStringLiteral("host-os")) {
            hostOsVal = postfixToFrontendFileSystemValue(postfix);
        } else if (symbolicName == QStringLiteral("host-etc")) {
            hostEtcVal = postfixToFrontendFileSystemValue(postfix);
        } else {
            name = symbolicName.toString();
            description = name;
            const QString fileSystemValue = postfixToFrontendFileSystemValue(postfix);
            isEnabledByDefault = true;
            filesysTemp.append(
                FlatpakPermission(name, category, description, FlatpakPermission::Filesystems, isEnabledByDefault, fileSystemValue, possibleValues));
        }
    }

    name = QStringLiteral("home");
    description = i18n("All User Files");
    possibleValues.removeAll(homeVal);
    if (homeVal == i18n("OFF")) {
        isEnabledByDefault = false;
        homeVal = i18n("read/write");
    } else {
        isEnabledByDefault = true;
    }
    m_permissions.insert(basicIndex++,
                         FlatpakPermission(name, category, description, FlatpakPermission::Filesystems, isEnabledByDefault, homeVal, possibleValues));

    name = QStringLiteral("host");
    description = i18n("All System Files");
    if (hostVal == i18n("OFF")) {
        isEnabledByDefault = false;
        hostVal = i18n("read/write");
    } else {
        isEnabledByDefault = true;
    }
    m_permissions.insert(basicIndex++,
                         FlatpakPermission(name, category, description, FlatpakPermission::Filesystems, isEnabledByDefault, hostVal, possibleValues));

    name = QStringLiteral("host-os");
    description = i18n("All System Libraries, Executables and Binaries");
    if (hostOsVal == i18n("OFF")) {
        isEnabledByDefault = false;
        hostOsVal = i18n("read/write");
    } else {
        isEnabledByDefault = true;
    }
    m_permissions.insert(basicIndex++,
                         FlatpakPermission(name, category, description, FlatpakPermission::Filesystems, isEnabledByDefault, hostOsVal, possibleValues));

    name = QStringLiteral("host-etc");
    description = i18n("All System Configurations");
    if (hostEtcVal == i18n("OFF")) {
        isEnabledByDefault = false;
        hostEtcVal = i18n("read/write");
    } else {
        isEnabledByDefault = true;
    }
    m_permissions.insert(basicIndex++,
                         FlatpakPermission(name, category, description, FlatpakPermission::Filesystems, isEnabledByDefault, hostEtcVal, possibleValues));

    for (int i = 0; i < filesysTemp.length(); i++) {
        m_permissions.insert(basicIndex++, filesysTemp.at(i));
    }
    /* FILESYSTEM category */

    /* DUMMY ADVANCED category */
    FlatpakPermission perm(QStringLiteral("Advanced Dummy"), QStringLiteral("Advanced Dummy"));
    perm.setPType(FlatpakPermission::Dummy);
    perm.setSType(FlatpakPermission::Advanced);
    m_permissions.insert(basicIndex++, perm);
    /* DUMMY ADVANCED category */

    /* SESSION BUS category */
    category = QStringLiteral("Session Bus Policy");
    const KConfigGroup sessionBusGroup = parser.group("Session Bus Policy");
    possibleValues.clear();
    possibleValues << i18n("talk") << i18n("own") << i18n("see");
    if (sessionBusGroup.exists()) {
        const QMap<QString, QString> busMap = sessionBusGroup.entryMap();
        const QStringList busList = busMap.keys();
        for (int i = 0; i < busList.length(); ++i) {
            name = description = busList.at(i);
            defaultValue = toFrontendDBusValue(busMap.value(busList.at(i)));
            isEnabledByDefault = true;
            //            m_permissions.append(FlatpakPermission(name, category, description, defaultValue, possibleValues, defaultValue,
            //            FlatpakPermission::ValueType::Complex));
            m_permissions.append(FlatpakPermission(name, category, description, FlatpakPermission::Bus, isEnabledByDefault, defaultValue, possibleValues));
        }
    } else {
        FlatpakPermission perm(QStringLiteral("Session Bus Dummy"), QStringLiteral("Session Bus Policy"));
        perm.setSType(FlatpakPermission::SectionType::Advanced);
        perm.setPType(FlatpakPermission::Dummy);
        m_permissions.append(perm);
    }
    /* SESSION BUS category */

    /* SYSTEM BUS category */
    category = QStringLiteral("System Bus Policy");
    const KConfigGroup systemBusGroup = parser.group("System Bus Policy");
    possibleValues.clear();
    possibleValues << i18n("talk") << i18n("own") << i18n("see");
    if (systemBusGroup.exists()) {
        const QMap<QString, QString> busMap = systemBusGroup.entryMap();
        const QStringList busList = busMap.keys();
        for (int i = 0; i < busList.length(); ++i) {
            name = description = busList.at(i);
            defaultValue = toFrontendDBusValue(busMap.value(busList.at(i)));
            isEnabledByDefault = true;
            m_permissions.append(FlatpakPermission(name, category, description, FlatpakPermission::Bus, isEnabledByDefault, defaultValue, possibleValues));
        }
    } else {
        FlatpakPermission perm(QStringLiteral("System Bus Dummy"), QStringLiteral("System Bus Policy"));
        perm.setSType(FlatpakPermission::SectionType::Advanced);
        perm.setPType(FlatpakPermission::Dummy);
        m_permissions.append(perm);
    }
    /* SYSTEM BUS category */

#if 0 // Disabled because BUG 465502
    /* ENVIRONMENT category */
    category = QStringLiteral("Environment");
    const KConfigGroup environmentGroup = parser.group("Environment");
    possibleValues.clear();
    if (environmentGroup.exists()) {
        const QMap<QString, QString> busMap = environmentGroup.entryMap();
        const QStringList busList = busMap.keys();
        for (int i = 0; i < busList.length(); ++i) {
            name = description = busList.at(i);
            defaultValue = busMap.value(busList.at(i));
            m_permissions.append(FlatpakPermission(name, category, description, FlatpakPermission::Environment, true, defaultValue));
        }
    } else {
        FlatpakPermission perm(QStringLiteral("Environment Dummy"), QStringLiteral("Environment"));
        perm.setSType(FlatpakPermission::SectionType::Advanced);
        perm.setPType(FlatpakPermission::Dummy);
        m_permissions.append(perm);
    }
    /* ENVIRONMENT category */
#endif
}

void FlatpakPermissionModel::loadCurrentValues()
{
    const QString path = m_reference->permissionsFilename();

    /* all permissions are at default, so nothing to load */
    if (!QFileInfo::exists(path)) {
        return;
    }

    KDesktopFile parser(path);
    const KConfigGroup contextGroup = parser.group("Context");

    int fsIndex = -1;

    for (int i = 0; i < m_permissions.length(); ++i) {
        FlatpakPermission *perm = &m_permissions[i];

        if (perm->type() == FlatpakPermission::Simple) {
            const QString cat = contextGroup.readEntry(perm->category(), QString());
            if (cat.contains(perm->name())) {
                // perm->setEnabled(!perm->enabledByDefault());
                bool isEnabled = !perm->enabledByDefault();
                perm->setEnabled(isEnabled);
                perm->setLoadEnabled(isEnabled);
            }
        } else if (perm->type() == FlatpakPermission::Filesystems) {
            const QString cat = contextGroup.readEntry(perm->category(), QString());
            if (cat.contains(perm->name())) {
                int permIndex = cat.indexOf(perm->name());

                /* the permission is just being set off, the access level isn't changed */
                bool isEnabled = permIndex > 0 ? cat.at(permIndex - 1) != QLatin1Char('!') : true;
                perm->setEnabled(isEnabled);
                perm->setLoadEnabled(isEnabled);

                int valueIndex = permIndex + perm->name().length();
                QString val;

                if (valueIndex >= cat.length() || cat.at(valueIndex) != QLatin1Char(':')) {
                    val = i18n("read/write");
                    perm->setCurrentValue(val);
                    perm->setLoadValue(val);
                    continue;
                }

                if (cat[valueIndex + 1] == QLatin1Char('r')) {
                    if (cat[valueIndex + 2] == QLatin1Char('w')) {
                        val = i18n("read/write");
                    } else {
                        val = i18n("read-only");
                    }
                } else if (cat[valueIndex + 1] == QLatin1Char('c')) {
                    val = i18n("create");
                }
                perm->setCurrentValue(val);
                perm->setLoadValue(val);
            }
            fsIndex = i;
        } else if (perm->type() == FlatpakPermission::Bus || perm->type() == FlatpakPermission::Environment) {
            const KConfigGroup group = parser.group(perm->category());
            if (group.exists()) {
                QMap<QString, QString> map = group.entryMap();
                QList<QString> list = map.keys();
                int index = list.indexOf(perm->name());
                if (index != -1) {
                    bool enabled = true;
                    QString value = map.value(list.at(index));
                    QString offSig = perm->type() == FlatpakPermission::Bus ? QStringLiteral("None") : QString();
                    if (value == offSig) {
                        enabled = false;
                        value = perm->currentValue();
                    } else {
                        value = toFrontendDBusValue(value);
                    }

                    perm->setCurrentValue(value);
                    perm->setLoadValue(value);
                    perm->setEnabled(enabled);
                    perm->setLoadEnabled(enabled);
                }
            }
        }
    }

    const QString fsCat = contextGroup.readEntry(QStringLiteral("filesystems"), QString());
    if (!fsCat.isEmpty()) {
        const QStringList fsPerms = fsCat.split(QLatin1Char(';'), Qt::SkipEmptyParts);
        for (int j = 0; j < fsPerms.length(); ++j) {
            QString name = fsPerms.at(j);
            QString value;
            int len = 0;
            bool enabled = false;
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
                QStringList possibleValues;
                possibleValues << i18n("read/write") << i18n("read-only") << i18n("create");
                m_permissions.insert(
                    fsIndex,
                    FlatpakPermission(name, QStringLiteral("filesystems"), name, FlatpakPermission::Filesystems, false, value, possibleValues));
                m_permissions[fsIndex].setEnabled(enabled);
                m_permissions[fsIndex].setLoadEnabled(enabled);
                m_permissions[fsIndex].setLoadValue(value);
                fsIndex++;
            }
        }
    }

    QVector<QString> cats = {QStringLiteral("Session Bus Policy"), QStringLiteral("System Bus Policy"), QStringLiteral("Environment")};
    for (int j = 0; j < cats.length(); j++) {
        const KConfigGroup group = parser.group(cats.at(j));
        if (!group.exists()) {
            continue;
        }
        int insertIndex = permIndex(cats.at(j));

        QMap<QString, QString> map = group.entryMap();
        QList<QString> list = map.keys();
        for (int k = 0; k < list.length(); k++) {
            if (!permExists(list.at(k))) {
                QString name = list.at(k);
                QString value = map.value(name);
                bool enabled = (j == 0 || j == 1) ? value != i18n("None") : !value.isEmpty();
                if (j == 0 || j == 1) {
                    QStringList possibleValues;
                    possibleValues << i18n("talk") << i18n("own") << i18n("see");
                    m_permissions.insert(insertIndex, FlatpakPermission(name, cats[j], name, FlatpakPermission::Bus, false, value, possibleValues));
                } else {
                    m_permissions.insert(insertIndex, FlatpakPermission(name, cats[j], name, FlatpakPermission::Environment, false, value));
                }
                m_permissions[insertIndex].setEnabled(enabled);
                m_permissions[insertIndex].setLoadEnabled(enabled);
                m_permissions[insertIndex].setLoadValue(value);
                insertIndex++;
            }
        }
    }
}

FlatpakReference *FlatpakPermissionModel::reference()
{
    return m_reference;
}

void FlatpakPermissionModel::load()
{
    m_permissions.clear();
    m_overridesData.clear();
    readFromFile();
    loadDefaultValues();
    loadCurrentValues();
    Q_EMIT dataChanged(FlatpakPermissionModel::index(0), FlatpakPermissionModel::index(m_permissions.length() - 1));
}

void FlatpakPermissionModel::save()
{
    for (auto &permission : m_permissions) {
        permission.setLoadEnabled(permission.enabled());
        if (permission.type() != FlatpakPermission::Simple) {
            permission.setLoadValue(permission.currentValue());
        }
    }
    writeToFile();
}

void FlatpakPermissionModel::defaults()
{
    m_overridesData.clear();
    for (auto &permission : m_permissions) {
        permission.setEnabled(permission.enabledByDefault());
        if (permission.type() != FlatpakPermission::Simple) {
            permission.setCurrentValue(permission.defaultValue());
        }
    }
    Q_EMIT dataChanged(FlatpakPermissionModel::index(0, 0), FlatpakPermissionModel::index(m_permissions.length() - 1, 0));
}

bool FlatpakPermissionModel::isDefaults() const
{
    return std::all_of(m_permissions.constBegin(), m_permissions.constEnd(), [](FlatpakPermission perm) {
        return perm.isDefaults();
    });
}

bool FlatpakPermissionModel::isSaveNeeded() const
{
    return std::any_of(m_permissions.begin(), m_permissions.end(), [](FlatpakPermission perm) {
        return perm.isSaveNeeded();
    });
}

void FlatpakPermissionModel::setReference(FlatpakReference *reference)
{
    if (reference != m_reference) {
        beginResetModel();
        if (m_reference) {
            m_reference->setPermsModel(nullptr);
        }
        m_reference = reference;
        if (m_reference) {
            m_reference->setPermsModel(this);
        }
        endResetModel();
        Q_EMIT referenceChanged();
    }
}

void FlatpakPermissionModel::setPerm(int index)
{
    /* guard for invalid indices */
    if (index < 0 || index >= m_permissions.length()) {
        return;
    }

    FlatpakPermission *perm = &m_permissions[index];
    bool isGranted = perm->enabled();

    if (perm->type() == FlatpakPermission::Simple) {
        bool setToNonDefault = (perm->enabledByDefault() && isGranted) || (!perm->enabledByDefault() && !isGranted);
        if (setToNonDefault) {
            addPermission(perm, !isGranted);
        } else {
            removePermission(perm, isGranted);
        }
        perm->setEnabled(!perm->enabled());
    } else if (perm->type() == FlatpakPermission::Filesystems) {
        if (perm->enabledByDefault() && isGranted) {
            /* if access level ("value") was changed, there's already an entry. Prepend ! to it.
             * if access level is unchanged, add a new entry */
            if (perm->defaultValue() != perm->currentValue()) {
                int permIndex = m_overridesData.indexOf(perm->name());
                m_overridesData.insert(permIndex, QLatin1Char('!'));
            } else {
                addPermission(perm, !isGranted);
            }
        } else if (!perm->enabledByDefault() && !isGranted) {
            if (!m_overridesData.contains(perm->name())) {
                addPermission(perm, !isGranted);
            }
            if (perm->pType() == FlatpakPermission::UserDefined) {
                m_overridesData.remove(m_overridesData.indexOf(perm->name()) - 1, 1);
            }
            // set value to read/write automatically, if not done already
        } else if (perm->enabledByDefault() && !isGranted) {
            /* if access level ("value") was changed, just remove ! from beginning.
             * if access level is unchanged, remove the whole entry */
            if (perm->defaultValue() != perm->currentValue()) {
                int permIndex = m_overridesData.indexOf(perm->name());
                m_overridesData.remove(permIndex - 1, 1);
            } else {
                removePermission(perm, isGranted);
            }
        } else if (!perm->enabledByDefault() && isGranted) {
            removePermission(perm, isGranted);
        }
        perm->setEnabled(!perm->enabled());
    } else if (perm->type() == FlatpakPermission::Bus) {
        if (perm->enabledByDefault() && isGranted) {
            perm->setEnabled(false);
            if (perm->defaultValue() != perm->currentValue()) {
                editBusPermissions(perm, i18n("None"));
            } else {
                addBusPermissions(perm);
            }
        } else if (perm->enabledByDefault() && !isGranted) {
            if (perm->defaultValue() != perm->currentValue()) {
                editBusPermissions(perm, perm->currentValue());
            } else {
                removeBusPermission(perm);
            }
            perm->setEnabled(true);
        } else if (!perm->enabledByDefault() && isGranted) {
            perm->setEnabled(false);
            removeBusPermission(perm);
        } else if (!perm->enabledByDefault() && !isGranted) {
            perm->setEnabled(true);
            addBusPermissions(perm);
        }
    } else if (perm->type() == FlatpakPermission::Environment) {
        if (perm->enabledByDefault() && isGranted) {
            perm->setEnabled(false);
            if (perm->defaultValue() != perm->currentValue()) {
                editEnvPermission(perm, QString());
            } else {
                addEnvPermission(perm);
            }
        } else if (perm->enabledByDefault() && !isGranted) {
            if (perm->defaultValue() != perm->currentValue()) {
                editEnvPermission(perm, perm->currentValue());
            } else {
                removeEnvPermission(perm);
            }
            perm->setEnabled(true);
        } else if (!perm->enabledByDefault() && isGranted) {
            perm->setEnabled(false);
            removeEnvPermission(perm);
        } else if (!perm->enabledByDefault() && !isGranted) {
            perm->setEnabled(true);
            addEnvPermission(perm);
        }
    }

    Q_EMIT dataChanged(FlatpakPermissionModel::index(index, 0), FlatpakPermissionModel::index(index, 0));
}

void FlatpakPermissionModel::editPerm(int index, const QString &newValue)
{
    /* guard for out-of-range indices */
    if (index < 0 || index >= m_permissions.length()) {
        return;
    }

    FlatpakPermission *perm = &m_permissions[index];

    /* editing the permission */
    if (perm->category() == QStringLiteral("filesystems")) {
        editFilesystemsPermissions(perm, newValue);
    } else if (perm->type() == FlatpakPermission::Bus) {
        editBusPermissions(perm, newValue);
    } else if (perm->type() == FlatpakPermission::Environment) {
        editEnvPermission(perm, newValue);
    }

    Q_EMIT dataChanged(FlatpakPermissionModel::index(index, 0), FlatpakPermissionModel::index(index, 0));
}

void FlatpakPermissionModel::addUserEnteredPermission(const QString &name, QString cat, const QString &value)
{
    QStringList possibleValues = valueList(cat);

    if (cat == i18n("Filesystem Access")) {
        cat = QStringLiteral("filesystems");
    }

    FlatpakPermission::ValueType type = FlatpakPermission::Environment;
    if (cat == QStringLiteral("filesystems")) {
        type = FlatpakPermission::Filesystems;
    } else if (cat == QStringLiteral("Session Bus Policy") || cat == QStringLiteral("System Bus Policy")) {
        type = FlatpakPermission::Bus;
    }

    FlatpakPermission perm(name, cat, name, type, false, QString(), possibleValues);
    perm.setPType(FlatpakPermission::UserDefined);
    perm.setEnabled(true);
    perm.setCurrentValue(value);

    int index = permIndex(cat);
    m_permissions.insert(index, perm);

    if (type == FlatpakPermission::Filesystems) {
        addPermission(&perm, true);
    } else if (type == FlatpakPermission::Bus) {
        addBusPermissions(&perm);
    } else {
        addEnvPermission(&perm);
    }

    Q_EMIT dataChanged(FlatpakPermissionModel::index(index, 0), FlatpakPermissionModel::index(index, 0));
}

QStringList FlatpakPermissionModel::valueList(const QString &catHeader) const
{
    QStringList valueList;
    if (catHeader == i18n("Filesystem Access")) {
        valueList << i18n("read/write") << i18n("read-only") << i18n("create");
    } else if (catHeader == i18n("Session Bus Policy") || catHeader == i18n("System Bus Policy")) {
        valueList << i18n("talk") << i18n("own") << i18n("see");
    }
    return valueList;
}

void FlatpakPermissionModel::addPermission(FlatpakPermission *perm, bool shouldBeOn)
{
    if (!m_overridesData.contains(QStringLiteral("[Context]"))) {
        m_overridesData.insert(m_overridesData.length(), QStringLiteral("[Context]\n"));
    }
    int catIndex = m_overridesData.indexOf(perm->category());

    /* if no permission from this category has been changed from default, we need to add the category */
    if (catIndex == -1) {
        catIndex = m_overridesData.indexOf(QLatin1Char('\n'), m_overridesData.indexOf(QStringLiteral("[Context]"))) + 1;
        if (catIndex == m_overridesData.length()) {
            m_overridesData.append(perm->category() + i18n("=\n"));
        } else {
            m_overridesData.insert(catIndex, perm->category() + i18n("=\n"));
        }
    }
    QString name = perm->name(); /* the name of the permission we are about to set/unset */
    if (perm->type() == FlatpakPermission::ValueType::Filesystems) {
        name.append(QLatin1Char(':') + perm->fsCurrentValue());
    }
    int permIndex = catIndex + perm->category().length() + 1;

    /* if there are other permissions in this category, we must add a ';' to separate this from the other */
    if (m_overridesData[permIndex] != QLatin1Char('\n')) {
        name.append(QLatin1Char(';'));
    }
    /* if permission was granted before user clicked to change it, we must un-grant it. And vice-versa */
    if (!shouldBeOn) {
        name.prepend(QLatin1Char('!'));
    }

    if (permIndex >= m_overridesData.length()) {
        m_overridesData.append(name);
    } else {
        m_overridesData.insert(permIndex, name);
    }
}

void FlatpakPermissionModel::removePermission(FlatpakPermission *perm, bool isGranted)
{
    int permStartIndex = m_overridesData.indexOf(perm->name());
    int permEndIndex = permStartIndex + perm->name().length();

    if (permStartIndex == -1) {
        return;
    }

    /* if it is not granted, there exists a ! one index before the name that should also be deleted */
    if (!isGranted) {
        permStartIndex--;
    }

    if (perm->type() == FlatpakPermission::ValueType::Filesystems) {
        if (m_overridesData[permEndIndex] == QLatin1Char(':')) {
            permEndIndex += perm->fsCurrentValue().length() + 1;
        }
    }

    /* if last permission in the list, include ';' of 2nd last too */
    if (m_overridesData[permEndIndex] != QLatin1Char(';')) {
        permEndIndex--;
        if (m_overridesData[permStartIndex - 1] == QLatin1Char(';')) {
            permStartIndex--;
        }
    }
    m_overridesData.remove(permStartIndex, permEndIndex - permStartIndex + 1);

    /* remove category entry if there are no more permission entries */
    int catIndex = m_overridesData.indexOf(perm->category());
    if (m_overridesData[m_overridesData.indexOf(QLatin1Char('='), catIndex) + 1] == QLatin1Char('\n')) {
        m_overridesData.remove(catIndex, perm->category().length() + 2); // 2 because we need to remove '\n' as well
    }
}

void FlatpakPermissionModel::addBusPermissions(FlatpakPermission *perm)
{
    QString groupHeader = QLatin1Char('[') + perm->category() + QLatin1Char(']');
    if (!m_overridesData.contains(groupHeader)) {
        m_overridesData.insert(m_overridesData.length(), groupHeader + QLatin1Char('\n'));
    }
    int permIndex = m_overridesData.indexOf(QLatin1Char('\n'), m_overridesData.indexOf(groupHeader)) + 1;
    QString val = perm->enabled() ? perm->currentValue() : i18n("None");
    m_overridesData.insert(permIndex, perm->name() + QLatin1Char('=') + val + QLatin1Char('\n'));
}

void FlatpakPermissionModel::removeBusPermission(FlatpakPermission *perm)
{
    int permBeginIndex = m_overridesData.indexOf(perm->name());
    if (permBeginIndex == -1) {
        return;
    }

    /* if it is enabled, the current value is not None. So get the length. Otherwise, it is 4 for None */
    int valueLen = perm->enabled() ? perm->currentValue().length() : 4;

    int permLen = perm->name().length() + 1 + valueLen + 1;
    m_overridesData.remove(permBeginIndex, permLen);
}

void FlatpakPermissionModel::editFilesystemsPermissions(FlatpakPermission *perm, const QString &newValue)
{
    QString value;
    if (newValue == i18n("read-only")) {
        value = QStringLiteral(":ro");
    } else if (newValue == i18n("create")) {
        value = QStringLiteral(":create");
    } else {
        value = QStringLiteral(":rw");
    }

    int permIndex = m_overridesData.indexOf(perm->name());
    if (permIndex == -1) {
        addPermission(perm, true);
        permIndex = m_overridesData.indexOf(perm->name());
    }

    if (perm->enabledByDefault() == perm->enabled() && perm->defaultValue() == newValue) {
        removePermission(perm, true);
        perm->setCurrentValue(newValue);
        return;
    }

    int valueBeginIndex = permIndex + perm->name().length();
    if (m_overridesData[valueBeginIndex] != QLatin1Char(':')) {
        m_overridesData.insert(permIndex + perm->name().length(), value);
    } else {
        m_overridesData.insert(valueBeginIndex, value);
        if (m_overridesData[valueBeginIndex + value.length() + 1] == QLatin1Char('r')) {
            m_overridesData.remove(valueBeginIndex + value.length(), 3);
        } else {
            m_overridesData.remove(valueBeginIndex + value.length(), 7);
        }
    }
    perm->setCurrentValue(newValue);
}

void FlatpakPermissionModel::editBusPermissions(FlatpakPermission *perm, const QString &l10nValue)
{
    if (perm->enabledByDefault() && l10nValue == perm->loadValue()) {
        perm->setCurrentValue(l10nValue);
        removeBusPermission(perm);
        return;
    }

    int permIndex = m_overridesData.indexOf(perm->name());
    if (permIndex == -1) {
        addBusPermissions(perm);
        permIndex = m_overridesData.indexOf(perm->name());
    }
    int valueBeginIndex = permIndex + perm->name().length();
    const QString backendValue = toBackendDBusValue(l10nValue);
    m_overridesData.insert(valueBeginIndex, QLatin1Char('=') + backendValue);
    valueBeginIndex = valueBeginIndex + 1 /* '=' character */ + backendValue.length();
    auto valueEndOfLine = m_overridesData.indexOf(QLatin1Char('\n'), valueBeginIndex);
    m_overridesData.remove(valueBeginIndex, valueEndOfLine - valueBeginIndex);
    if (l10nValue != i18n("None")) {
        perm->setCurrentValue(l10nValue);
    }
}

void FlatpakPermissionModel::addEnvPermission(FlatpakPermission *perm)
{
    QString groupHeader = QLatin1Char('[') + perm->category() + QLatin1Char(']');
    if (!m_overridesData.contains(groupHeader)) {
        m_overridesData.insert(m_overridesData.length(), groupHeader + QLatin1Char('\n'));
    }
    int permIndex = m_overridesData.indexOf(QLatin1Char('\n'), m_overridesData.indexOf(groupHeader)) + 1;
    QString val = perm->enabled() ? perm->currentValue() : QString();
    m_overridesData.insert(permIndex, perm->name() + QLatin1Char('=') + val + QLatin1Char('\n'));
}

void FlatpakPermissionModel::removeEnvPermission(FlatpakPermission *perm)
{
    int permBeginIndex = m_overridesData.indexOf(perm->name());
    if (permBeginIndex == -1) {
        return;
    }

    int valueLen = perm->enabled() ? perm->currentValue().length() + 1 : 0;

    int permLen = perm->name().length() + 1 + valueLen;
    m_overridesData.remove(permBeginIndex, permLen);
}

void FlatpakPermissionModel::editEnvPermission(FlatpakPermission *perm, const QString &value)
{
    if (perm->enabledByDefault() && value == perm->defaultValue()) {
        removeEnvPermission(perm);
        return;
    }

    int permIndex = m_overridesData.indexOf(perm->name());
    if (permIndex == -1) {
        addEnvPermission(perm);
        permIndex = m_overridesData.indexOf(perm->name());
    }
    int valueBeginIndex = permIndex + perm->name().length();
    m_overridesData.insert(valueBeginIndex, QLatin1Char('=') + value);

    int oldValBeginIndex = valueBeginIndex + value.length() + 1;
    int oldValEndIndex = m_overridesData.indexOf(QLatin1Char('\n'), oldValBeginIndex);
    m_overridesData.remove(oldValBeginIndex, oldValEndIndex - oldValBeginIndex);

    if (!value.isEmpty()) {
        perm->setCurrentValue(value);
    }
}

bool FlatpakPermissionModel::permExists(const QString &name)
{
    return std::any_of(m_permissions.cbegin(), m_permissions.cend(), [name](const FlatpakPermission &permission) {
        return permission.name() == name;
    });
}

int FlatpakPermissionModel::permIndex(const QString &category, int from)
{
    int i = from;
    while (i < m_permissions.length()) {
        if (m_permissions.at(i).category() == category) {
            break;
        }
        i++;
    }
    if (i < m_permissions.length()) {
        while (i < m_permissions.length()) {
            if (m_permissions.at(i).category() != category) {
                break;
            }
            i++;
        }
    }
    if (m_permissions.at(i - 1).pType() == FlatpakPermission::Dummy) {
        m_permissions.remove(i - 1, 1);
        i--;
    }
    return i;
}

void FlatpakPermissionModel::readFromFile()
{
    if (!m_overridesData.isEmpty()) {
        return;
    }

    QFile inFile(m_reference->permissionsFilename());
    if (!inFile.open(QIODevice::ReadOnly)) {
        return;
    }
    QTextStream inStream(&inFile);
    m_overridesData = inStream.readAll();
    inFile.close();
}

void FlatpakPermissionModel::writeToFile()
{
    QFile outFile(m_reference->permissionsFilename());
    if (!outFile.open(QIODevice::WriteOnly)) {
        qInfo() << "File does not open for write only";
    }
    QTextStream outStream(&outFile);
    outStream << m_overridesData;
    outFile.close();
}
