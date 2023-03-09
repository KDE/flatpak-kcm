/**
 * SPDX-FileCopyrightText: 2022 Suhaas Joshi <joshiesuhaas0@gmail.com>
 * SPDX-FileCopyrightText: 2023 Harald Sitter <sitter@kde.org>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "flatpakpermission.h"
#include "flatpakcommon.h"

#include <KConfig>
#include <KConfigGroup>
#include <KLocalizedString>
#include <QDebug>
#include <QFileInfo>
#include <QMetaEnum>
#include <QTemporaryFile>

#include <array>

FlatpakPermission::ValueType FlatpakPermission::valueTypeFromSectionType(FlatpakPermissionsSectionType::Type section)
{
    switch (section) {
    case FlatpakPermissionsSectionType::Filesystems:
        return FlatpakPermission::ValueType::Filesystems;
    case FlatpakPermissionsSectionType::SessionBus:
    case FlatpakPermissionsSectionType::SystemBus:
        return FlatpakPermission::ValueType::Bus;
    case FlatpakPermissionsSectionType::Environment:
        return FlatpakPermission::ValueType::Environment;
    case FlatpakPermissionsSectionType::Basic:
    case FlatpakPermissionsSectionType::Advanced:
    case FlatpakPermissionsSectionType::SubsystemsShared:
    case FlatpakPermissionsSectionType::Sockets:
    case FlatpakPermissionsSectionType::Devices:
    case FlatpakPermissionsSectionType::Features:
        break;
    }
    return FlatpakPermission::ValueType::Simple;
}

FlatpakPermission::FlatpakPermission(FlatpakPermissionsSectionType::Type section)
    : FlatpakPermission(section, QString(), QString(), QString(), false)
{
    m_originType = OriginType::Dummy;
}

FlatpakPermission::FlatpakPermission(FlatpakPermissionsSectionType::Type section,
                                     const QString &name,
                                     const QString &category,
                                     const QString &description,
                                     bool isDefaultEnabled,
                                     const QString &defaultValue,
                                     const QStringList &possibleValues)
    : m_section(section)
    , m_name(name)
    , m_category(category)
    , m_description(description)
    //
    , m_originType(OriginType::BuiltIn)
    //
    , m_defaultEnable(isDefaultEnabled)
    , m_overrideEnable(isDefaultEnabled)
    , m_effectiveEnable(isDefaultEnabled)
    //
    , m_defaultValue(defaultValue)
    , m_overrideValue(defaultValue)
    , m_effectiveValue(defaultValue)
    //
    , m_possibleValues(possibleValues)
{
}

FlatpakPermissionsSectionType::Type FlatpakPermission::section() const
{
    return m_section;
}

const QString &FlatpakPermission::name() const
{
    return m_name;
}

const QString &FlatpakPermission::category() const
{
    return m_category;
}

const QString &FlatpakPermission::description() const
{
    return m_description;
}

FlatpakPermission::ValueType FlatpakPermission::valueType() const
{
    return valueTypeFromSectionType(m_section);
}

FlatpakPermission::OriginType FlatpakPermission::originType() const
{
    return m_originType;
}

void FlatpakPermission::setOriginType(OriginType type)
{
    m_originType = type;
}

bool FlatpakPermission::isDefaultEnabled() const
{
    return m_defaultEnable;
}

void FlatpakPermission::setOverrideEnabled(bool enabled)
{
    m_overrideEnable = enabled;
}

bool FlatpakPermission::isEffectiveEnabled() const
{
    return m_effectiveEnable;
}

void FlatpakPermission::setEffectiveEnabled(bool enabled)
{
    m_effectiveEnable = enabled;
}

const QString &FlatpakPermission::defaultValue() const
{
    return m_defaultValue;
}

const QString &FlatpakPermission::overrideValue() const
{
    return m_overrideValue;
}

void FlatpakPermission::setOverrideValue(const QString &value)
{
    m_overrideValue = value;
}

const QString &FlatpakPermission::effectiveValue() const
{
    return m_effectiveValue;
}

void FlatpakPermission::setEffectiveValue(const QString &value)
{
    m_effectiveValue = value;
}

QString FlatpakPermission::fsCurrentValue() const
{
    // NB: the use of i18n here is actually kind of wrong but at the time of writing fixing the mapping
    // between ui and backend *here* is easier/safer than trying to reinvent the way the mapping works in a
    // more reliable fashion.
    if (m_effectiveValue == i18n("OFF")) {
        return QString();
    }
    if (m_effectiveValue == i18n("read-only")) {
        return QLatin1String("ro");
    }
    if (m_effectiveValue == i18n("create")) {
        return QLatin1String("create");
    }
    return QLatin1String("rw");
}

const QStringList &FlatpakPermission::possibleValues() const
{
    return m_possibleValues;
}

bool FlatpakPermission::isSaveNeeded() const
{
    if (m_originType == FlatpakPermission::OriginType::Dummy) {
        return false;
    }

    const bool enableDiffers = m_effectiveEnable != m_overrideEnable;
    if (valueType() != FlatpakPermission::ValueType::Simple) {
        const bool valueDiffers = m_effectiveValue != m_overrideValue;
        return enableDiffers || valueDiffers;
    }
    return enableDiffers;
}

bool FlatpakPermission::isDefaults() const
{
    if (m_originType == FlatpakPermission::OriginType::Dummy) {
        return true;
    }

    const bool enableIsTheSame = m_effectiveEnable == m_defaultEnable;
    if (valueType() != FlatpakPermission::ValueType::Simple) {
        const bool valueIsTheSame = m_effectiveValue == m_defaultValue;
        return enableIsTheSame && valueIsTheSame;
    }
    return enableIsTheSame;
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
    if (value == QStringLiteral("none")) {
        return i18n("None");
    }
    Q_ASSERT_X(false, Q_FUNC_INFO, qUtf8Printable(QStringLiteral("unmapped value %1").arg(value)));
    return QString();
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
        return QStringLiteral("none");
    }
    Q_ASSERT_X(false, Q_FUNC_INFO, qUtf8Printable(QStringLiteral("unmapped value %1").arg(value)));
    return QString();
}

static QString postfixToFrontendFileSystemValue(const QStringView &postfix)
{
    if (postfix == QLatin1String(":ro")) {
        return i18n("read-only");
    }
    if (postfix == QLatin1String(":create")) {
        return i18n("create");
    }
    return i18n("read/write");
}

FlatpakPermissionModel::FlatpakPermissionModel(QObject *parent)
    : QAbstractListModel(parent)
    , m_showAdvanced(false)
{
}

int FlatpakPermissionModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return rowCount(m_showAdvanced);
}

QVariant FlatpakPermissionModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }

    const auto permission = m_permissions.at(index.row());

    switch (role) {
    case Roles::Section:
        return permission.section();
    case Roles::Name:
        return permission.name();
    case Roles::Description:
        return permission.description();
    //
    case Roles::IsSimple:
        return permission.valueType() == FlatpakPermission::ValueType::Simple;
    case Roles::IsEnvironment:
        return permission.valueType() == FlatpakPermission::ValueType::Environment;
    case Roles::IsNotDummy:
        return permission.originType() != FlatpakPermission::OriginType::Dummy;
    //
    case Roles::IsDefaultEnabled:
        return permission.isDefaultEnabled();
    case Roles::IsEffectiveEnabled:
        return permission.isEffectiveEnabled();
    case Roles::DefaultValue:
        return permission.defaultValue();
    case Roles::EffectiveValue:
        return permission.effectiveValue();
    //
    case Roles::ValueList:
        return permission.possibleValues();
    }

    return QVariant();
}

QHash<int, QByteArray> FlatpakPermissionModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    //
    roles[Roles::Section] = "section";
    roles[Roles::Name] = "name";
    roles[Roles::Description] = "description";
    //
    roles[Roles::IsSimple] = "isSimple";
    roles[Roles::IsEnvironment] = "isEnvironment";
    roles[Roles::IsNotDummy] = "isNotDummy";
    //
    roles[Roles::IsDefaultEnabled] = "isDefaultEnabled";
    roles[Roles::IsEffectiveEnabled] = "isEffectiveEnabled";
    roles[Roles::DefaultValue] = "defaultValue";
    roles[Roles::EffectiveValue] = "effectiveValue";
    //
    roles[Roles::ValueList] = "valueList";
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

    KConfig parser(f.fileName());
    const KConfigGroup contextGroup = parser.group(FLATPAK_METADATA_GROUP_CONTEXT);

    int basicIndex = 0;

    /* SHARED category */
    category = QLatin1String(FLATPAK_METADATA_KEY_SHARED);
    const QString sharedPerms = contextGroup.readEntry(QLatin1String(FLATPAK_METADATA_KEY_SHARED), QString());

    name = QStringLiteral("network");
    description = i18n("Internet Connection");
    isEnabledByDefault = sharedPerms.contains(name);
    m_permissions.insert(basicIndex, FlatpakPermission(FlatpakPermissionsSectionType::Basic, name, category, description, isEnabledByDefault));
    basicIndex += 1;

    name = QStringLiteral("ipc");
    description = i18n("Inter-process Communication");
    isEnabledByDefault = sharedPerms.contains(name);
    m_permissions.append(FlatpakPermission(FlatpakPermissionsSectionType::SubsystemsShared, name, category, description, isEnabledByDefault));
    /* SHARED category */

    /* SOCKETS category */
    category = QLatin1String(FLATPAK_METADATA_KEY_SOCKETS);
    const QString socketPerms = contextGroup.readEntry(QLatin1String(FLATPAK_METADATA_KEY_SOCKETS), QString());

    name = QStringLiteral("x11");
    description = i18n("X11 Windowing System");
    isEnabledByDefault = socketPerms.contains(name);
    m_permissions.append(FlatpakPermission(FlatpakPermissionsSectionType::Sockets, name, category, description, isEnabledByDefault));

    name = QStringLiteral("wayland");
    description = i18n("Wayland Windowing System");
    isEnabledByDefault = socketPerms.contains(name);
    m_permissions.append(FlatpakPermission(FlatpakPermissionsSectionType::Sockets, name, category, description, isEnabledByDefault));

    name = QStringLiteral("fallback-x11");
    description = i18n("Fallback to X11 Windowing System");
    isEnabledByDefault = socketPerms.contains(name);
    m_permissions.append(FlatpakPermission(FlatpakPermissionsSectionType::Sockets, name, category, description, isEnabledByDefault));

    name = QStringLiteral("pulseaudio");
    description = i18n("Pulseaudio Sound Server");
    isEnabledByDefault = socketPerms.contains(name);
    m_permissions.insert(basicIndex, FlatpakPermission(FlatpakPermissionsSectionType::Basic, name, category, description, isEnabledByDefault));
    basicIndex += 1;

    name = QStringLiteral("session-bus");
    description = i18n("Session Bus Access");
    isEnabledByDefault = socketPerms.contains(name);
    m_permissions.append(FlatpakPermission(FlatpakPermissionsSectionType::Sockets, name, category, description, isEnabledByDefault));

    name = QStringLiteral("system-bus");
    description = i18n("System Bus Access");
    isEnabledByDefault = socketPerms.contains(name);
    m_permissions.append(FlatpakPermission(FlatpakPermissionsSectionType::Sockets, name, category, description, isEnabledByDefault));

    name = QStringLiteral("ssh-auth");
    description = i18n("Remote Login Access");
    isEnabledByDefault = socketPerms.contains(name);
    m_permissions.insert(basicIndex, FlatpakPermission(FlatpakPermissionsSectionType::Basic, name, category, description, isEnabledByDefault));
    basicIndex += 1;

    name = QStringLiteral("pcsc");
    description = i18n("Smart Card Access");
    isEnabledByDefault = socketPerms.contains(name);
    m_permissions.insert(basicIndex, FlatpakPermission(FlatpakPermissionsSectionType::Basic, name, category, description, isEnabledByDefault));
    basicIndex += 1;

    name = QStringLiteral("cups");
    description = i18n("Print System Access");
    isEnabledByDefault = socketPerms.contains(name);
    m_permissions.insert(basicIndex, FlatpakPermission(FlatpakPermissionsSectionType::Basic, name, category, description, isEnabledByDefault));
    basicIndex += 1;
    /* SOCKETS category */

    /* DEVICES category */
    category = QLatin1String(FLATPAK_METADATA_KEY_DEVICES);
    const QString devicesPerms = contextGroup.readEntry(QLatin1String(FLATPAK_METADATA_KEY_DEVICES), QString());

    name = QStringLiteral("kvm");
    description = i18n("Kernel-based Virtual Machine Access");
    isEnabledByDefault = devicesPerms.contains(name);
    m_permissions.append(FlatpakPermission(FlatpakPermissionsSectionType::Devices, name, category, description, isEnabledByDefault));

    name = QStringLiteral("dri");
    description = i18n("Direct Graphic Rendering");
    isEnabledByDefault = devicesPerms.contains(name);
    m_permissions.append(FlatpakPermission(FlatpakPermissionsSectionType::Devices, name, category, description, isEnabledByDefault));

    name = QStringLiteral("shm");
    description = i18n("Host dev/shm");
    isEnabledByDefault = devicesPerms.contains(name);
    m_permissions.append(FlatpakPermission(FlatpakPermissionsSectionType::Devices, name, category, description, isEnabledByDefault));

    name = QStringLiteral("all");
    description = i18n("Device Access");
    isEnabledByDefault = devicesPerms.contains(name);
    m_permissions.insert(basicIndex, FlatpakPermission(FlatpakPermissionsSectionType::Basic, name, category, description, isEnabledByDefault));
    basicIndex += 1;
    /* DEVICES category */

    /* FEATURES category */
    category = QLatin1String(FLATPAK_METADATA_KEY_FEATURES);
    const QString featuresPerms = contextGroup.readEntry(QLatin1String(FLATPAK_METADATA_KEY_FEATURES), QString());

    name = QStringLiteral("devel");
    description = i18n("System Calls by Development Tools");
    isEnabledByDefault = featuresPerms.contains(name);
    m_permissions.append(FlatpakPermission(FlatpakPermissionsSectionType::Features, name, category, description, isEnabledByDefault));

    name = QStringLiteral("multiarch");
    description = i18n("Run Multiarch/Multilib Binaries");
    isEnabledByDefault = featuresPerms.contains(name);
    m_permissions.append(FlatpakPermission(FlatpakPermissionsSectionType::Features, name, category, description, isEnabledByDefault));

    name = QStringLiteral("bluetooth");
    description = i18n("Bluetooth");
    isEnabledByDefault = featuresPerms.contains(name);
    m_permissions.insert(basicIndex, FlatpakPermission(FlatpakPermissionsSectionType::Basic, name, category, description, isEnabledByDefault));
    basicIndex += 1;

    name = QStringLiteral("canbus");
    description = i18n("Canbus Socket Access");
    isEnabledByDefault = featuresPerms.contains(name);
    m_permissions.append(FlatpakPermission(FlatpakPermissionsSectionType::Features, name, category, description, isEnabledByDefault));

    name = QStringLiteral("per-app-dev-shm");
    description = i18n("Share dev/shm across all instances of an app per user ID");
    isEnabledByDefault = featuresPerms.contains(name);
    m_permissions.append(FlatpakPermission(FlatpakPermissionsSectionType::Features, name, category, description, isEnabledByDefault));
    /* FEATURES category */

    /* FILESYSTEM category */
    category = QLatin1String(FLATPAK_METADATA_KEY_FILESYSTEMS);
    const QString fileSystemPerms = contextGroup.readEntry(QLatin1String(FLATPAK_METADATA_KEY_FILESYSTEMS), QString());
    const auto dirs = QStringView(fileSystemPerms).split(QLatin1Char(';'), Qt::SkipEmptyParts);

    QString homeVal = i18n("OFF");
    QString hostVal = i18n("OFF");
    QString hostOsVal = i18n("OFF");
    QString hostEtcVal = i18n("OFF");
    possibleValues = valueListForSectionType(FlatpakPermissionsSectionType::Filesystems);

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
            filesysTemp.append(FlatpakPermission(FlatpakPermissionsSectionType::Filesystems,
                                                 name,
                                                 category,
                                                 description,
                                                 isEnabledByDefault,
                                                 fileSystemValue,
                                                 possibleValues));
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
    m_permissions.insert(
        basicIndex,
        FlatpakPermission(FlatpakPermissionsSectionType::Filesystems, name, category, description, isEnabledByDefault, homeVal, possibleValues));
    basicIndex += 1;

    name = QStringLiteral("host");
    description = i18n("All System Files");
    if (hostVal == i18n("OFF")) {
        isEnabledByDefault = false;
        hostVal = i18n("read/write");
    } else {
        isEnabledByDefault = true;
    }
    m_permissions.insert(
        basicIndex,
        FlatpakPermission(FlatpakPermissionsSectionType::Filesystems, name, category, description, isEnabledByDefault, hostVal, possibleValues));
    basicIndex += 1;

    name = QStringLiteral("host-os");
    description = i18n("All System Libraries, Executables and Binaries");
    if (hostOsVal == i18n("OFF")) {
        isEnabledByDefault = false;
        hostOsVal = i18n("read/write");
    } else {
        isEnabledByDefault = true;
    }
    m_permissions.insert(
        basicIndex,
        FlatpakPermission(FlatpakPermissionsSectionType::Filesystems, name, category, description, isEnabledByDefault, hostOsVal, possibleValues));
    basicIndex += 1;

    name = QStringLiteral("host-etc");
    description = i18n("All System Configurations");
    if (hostEtcVal == i18n("OFF")) {
        isEnabledByDefault = false;
        hostEtcVal = i18n("read/write");
    } else {
        isEnabledByDefault = true;
    }
    m_permissions.insert(
        basicIndex,
        FlatpakPermission(FlatpakPermissionsSectionType::Filesystems, name, category, description, isEnabledByDefault, hostEtcVal, possibleValues));
    basicIndex += 1;

    for (int i = 0; i < filesysTemp.length(); i++) {
        m_permissions.insert(basicIndex, filesysTemp.at(i));
        basicIndex += 1;
    }
    /* FILESYSTEM category */

    /* DUMMY ADVANCED category */
    m_permissions.insert(basicIndex, FlatpakPermission(FlatpakPermissionsSectionType::Advanced));
    basicIndex += 1;
    /* DUMMY ADVANCED category */

    /* SESSION BUS category */
    category = QLatin1String(FLATPAK_METADATA_GROUP_SESSION_BUS_POLICY);
    const KConfigGroup sessionBusGroup = parser.group(QLatin1String(FLATPAK_METADATA_GROUP_SESSION_BUS_POLICY));
    possibleValues = valueListForSectionType(FlatpakPermissionsSectionType::SessionBus);
    if (sessionBusGroup.exists() && !sessionBusGroup.entryMap().isEmpty()) {
        const QMap<QString, QString> busMap = sessionBusGroup.entryMap();
        const QStringList busList = busMap.keys();
        for (int i = 0; i < busList.length(); ++i) {
            name = description = busList.at(i);
            defaultValue = toFrontendDBusValue(busMap.value(busList.at(i)));
            isEnabledByDefault = true;
            m_permissions.append(
                FlatpakPermission(FlatpakPermissionsSectionType::SessionBus, name, category, description, isEnabledByDefault, defaultValue, possibleValues));
        }
    } else {
        m_permissions.append(FlatpakPermission(FlatpakPermissionsSectionType::SessionBus));
    }
    /* SESSION BUS category */

    /* SYSTEM BUS category */
    category = QLatin1String(FLATPAK_METADATA_GROUP_SYSTEM_BUS_POLICY);
    const KConfigGroup systemBusGroup = parser.group(QLatin1String(FLATPAK_METADATA_GROUP_SYSTEM_BUS_POLICY));
    possibleValues = valueListForSectionType(FlatpakPermissionsSectionType::SystemBus);
    if (systemBusGroup.exists() && !systemBusGroup.entryMap().isEmpty()) {
        const QMap<QString, QString> busMap = systemBusGroup.entryMap();
        const QStringList busList = busMap.keys();
        for (int i = 0; i < busList.length(); ++i) {
            name = description = busList.at(i);
            defaultValue = toFrontendDBusValue(busMap.value(busList.at(i)));
            isEnabledByDefault = true;
            m_permissions.append(
                FlatpakPermission(FlatpakPermissionsSectionType::SystemBus, name, category, description, isEnabledByDefault, defaultValue, possibleValues));
        }
    } else {
        m_permissions.append(FlatpakPermission(FlatpakPermissionsSectionType::SystemBus));
    }
    /* SYSTEM BUS category */

    // Disabled because BUG 465502
    if (false) {
        /* ENVIRONMENT category */
        category = QLatin1String(FLATPAK_METADATA_GROUP_ENVIRONMENT);
        const KConfigGroup environmentGroup = parser.group(QLatin1String(FLATPAK_METADATA_GROUP_ENVIRONMENT));
        possibleValues.clear();
        if (environmentGroup.exists() && !environmentGroup.entryMap().isEmpty()) {
            const QMap<QString, QString> busMap = environmentGroup.entryMap();
            const QStringList busList = busMap.keys();
            for (int i = 0; i < busList.length(); ++i) {
                name = description = busList.at(i);
                defaultValue = busMap.value(busList.at(i));
                m_permissions.append(FlatpakPermission(FlatpakPermissionsSectionType::Environment, name, category, description, true, defaultValue));
            }
        } else {
            m_permissions.append(FlatpakPermission(FlatpakPermissionsSectionType::Environment));
        }
        /* ENVIRONMENT category */
    } // end of if (false)
}

void FlatpakPermissionModel::loadCurrentValues()
{
    const QString path = m_reference->path();

    /* all permissions are at default, so nothing to load */
    if (!QFileInfo::exists(path)) {
        return;
    }

    KConfig parser(path);
    const KConfigGroup contextGroup = parser.group(FLATPAK_METADATA_GROUP_CONTEXT);

    int fsIndex = -1;

    for (int i = 0; i < m_permissions.length(); ++i) {
        FlatpakPermission *perm = &m_permissions[i];

        switch (perm->valueType()) {
        case FlatpakPermission::ValueType::Simple: {
            const QString cat = contextGroup.readEntry(perm->category(), QString());
            if (cat.contains(perm->name())) {
                // perm->setEnabled(!perm->enabledByDefault());
                bool isEnabled = !perm->isDefaultEnabled();
                perm->setEffectiveEnabled(isEnabled);
                perm->setOverrideEnabled(isEnabled);
            }
            break;
        }
        case FlatpakPermission::ValueType::Filesystems: {
            const QString cat = contextGroup.readEntry(perm->category(), QString());
            if (cat.contains(perm->name())) {
                int permIndex = cat.indexOf(perm->name());

                /* the permission is just being set off, the access level isn't changed */
                bool isEnabled = permIndex > 0 ? cat.at(permIndex - 1) != QLatin1Char('!') : true;
                perm->setEffectiveEnabled(isEnabled);
                perm->setOverrideEnabled(isEnabled);

                int valueIndex = permIndex + perm->name().length();
                QString val;

                if (valueIndex >= cat.length() || cat.at(valueIndex) != QLatin1Char(':')) {
                    val = i18n("read/write");
                    perm->setEffectiveValue(val);
                    perm->setOverrideValue(val);
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
                perm->setEffectiveValue(val);
                perm->setOverrideValue(val);
            }
            fsIndex = i + 1;
            break;
        }
        case FlatpakPermission::ValueType::Bus:
        case FlatpakPermission::ValueType::Environment: {
            const KConfigGroup group = parser.group(perm->category());
            if (group.exists()) {
                QMap<QString, QString> map = group.entryMap();
                QList<QString> list = map.keys();
                int index = list.indexOf(perm->name());
                if (index != -1) {
                    QString value = map.value(perm->name());
                    if (perm->valueType() == FlatpakPermission::ValueType::Bus) {
                        value = toFrontendDBusValue(value);
                    }

                    perm->setOverrideValue(value);
                    perm->setEffectiveValue(value);
                    perm->setOverrideEnabled(true);
                    perm->setEffectiveEnabled(true);
                }
            }
            break;
        }
        } // end of switch
    }

    const QString fsCat = contextGroup.readEntry(QLatin1String(FLATPAK_METADATA_KEY_FILESYSTEMS), QString());
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
                const auto possibleValues = valueListForSectionType(FlatpakPermissionsSectionType::Filesystems);
                m_permissions.insert(fsIndex,
                                     FlatpakPermission(FlatpakPermissionsSectionType::Filesystems,
                                                       name,
                                                       QLatin1String(FLATPAK_METADATA_KEY_FILESYSTEMS),
                                                       name,
                                                       false,
                                                       value,
                                                       possibleValues));
                m_permissions[fsIndex].setEffectiveEnabled(enabled);
                m_permissions[fsIndex].setOverrideEnabled(enabled);
                m_permissions[fsIndex].setOverrideValue(value);
                fsIndex++;
            }
        }
    }

    const std::array categories = {
        std::make_tuple(FlatpakPermissionsSectionType::SessionBus, QLatin1String(FLATPAK_METADATA_GROUP_SESSION_BUS_POLICY)),
        std::make_tuple(FlatpakPermissionsSectionType::SystemBus, QLatin1String(FLATPAK_METADATA_GROUP_SYSTEM_BUS_POLICY)),
        std::make_tuple(FlatpakPermissionsSectionType::Environment, QLatin1String(FLATPAK_METADATA_GROUP_ENVIRONMENT)),
    };
    for (const auto &[section, category] : categories) {
        if (section == FlatpakPermissionsSectionType::Environment) {
            // Disabled because BUG 465502
            continue;
        }
        const KConfigGroup group = parser.group(category);
        if (!group.exists()) {
            continue;
        }
        const auto valueType = FlatpakPermission::valueTypeFromSectionType(section);
        int insertIndex = permIndex(category);

        QMap<QString, QString> map = group.entryMap();
        QList<QString> list = map.keys();
        for (int k = 0; k < list.length(); k++) {
            if (!permExists(list.at(k))) {
                QString name = list.at(k);
                QString value = map.value(name);
                if (valueType == FlatpakPermission::ValueType::Bus) {
                    value = toFrontendDBusValue(value);
                    const auto possibleValues = valueListForSectionType(section);
                    m_permissions.insert(insertIndex, FlatpakPermission(section, name, category, name, false, value, possibleValues));
                } else {
                    m_permissions.insert(insertIndex, FlatpakPermission(section, name, category, name, false, value));
                }
                m_permissions[insertIndex].setOverrideEnabled(true);
                m_permissions[insertIndex].setEffectiveEnabled(true);
                insertIndex++;
            }
        }
    }
}

FlatpakReference *FlatpakPermissionModel::reference() const
{
    return m_reference;
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

bool FlatpakPermissionModel::showAdvanced() const
{
    return m_showAdvanced;
}

void FlatpakPermissionModel::setShowAdvanced(bool show)
{
    if (m_showAdvanced != show) {
        const int whenHidden = rowCount(false);
        const int whenShown = rowCount(true);

        if (show) {
            beginInsertRows(QModelIndex(), whenHidden, whenShown - 1);
        } else {
            beginRemoveRows(QModelIndex(), whenHidden, whenShown - 1);
        }

        m_showAdvanced = show;

        if (show) {
            endInsertRows();
        } else {
            endRemoveRows();
        }

        Q_EMIT showAdvancedChanged();
    }
}

int FlatpakPermissionModel::rowCount(bool showAdvanced) const
{
    if (showAdvanced) {
        return m_permissions.count();
    } else {
        int count = 0;
        for (const auto &permission : m_permissions) {
            if (permission.section() == FlatpakPermissionsSectionType::Basic || permission.section() == FlatpakPermissionsSectionType::Filesystems
                || permission.section() == FlatpakPermissionsSectionType::Advanced) {
                count += 1;
            } else {
                break;
            }
        }
        return count;
    }
}

void FlatpakPermissionModel::load()
{
    m_permissions.clear();
    m_overridesData.clear();
    readFromFile();
    loadDefaultValues();
    loadCurrentValues();
    Q_EMIT dataChanged(FlatpakPermissionModel::index(0, 0), FlatpakPermissionModel::index(rowCount() - 1, 0));
}

void FlatpakPermissionModel::save()
{
    for (auto &permission : m_permissions) {
        permission.setOverrideEnabled(permission.isEffectiveEnabled());
        if (permission.valueType() != FlatpakPermission::ValueType::Simple) {
            permission.setOverrideValue(permission.effectiveValue());
        }
    }
    writeToFile();
}

void FlatpakPermissionModel::defaults()
{
    m_overridesData.clear();
    for (auto &permission : m_permissions) {
        permission.setEffectiveEnabled(permission.isDefaultEnabled());
        if (permission.valueType() != FlatpakPermission::ValueType::Simple) {
            permission.setEffectiveValue(permission.defaultValue());
        }
    }
    Q_EMIT dataChanged(FlatpakPermissionModel::index(0, 0), FlatpakPermissionModel::index(rowCount() - 1, 0), QVector<int>{IsEffectiveEnabled, EffectiveValue});
}

bool FlatpakPermissionModel::isDefaults() const
{
    return std::all_of(m_permissions.constBegin(), m_permissions.constEnd(), [](const FlatpakPermission &permission) {
        return permission.isDefaults();
    });
}

bool FlatpakPermissionModel::isSaveNeeded() const
{
    return std::any_of(m_permissions.constBegin(), m_permissions.constEnd(), [](const FlatpakPermission &permission) {
        return permission.isSaveNeeded();
    });
}

QStringList FlatpakPermissionModel::valueListForSectionType(int /*FlatpakPermissionsSectionType::Type*/ rawSection) const
{
    if (!QMetaEnum::fromType<FlatpakPermissionsSectionType::Type>().valueToKey(rawSection)) {
        return {};
    }
    // SAFETY: QMetaEnum above ensures that coercion is valid.
    const auto section = static_cast<FlatpakPermissionsSectionType::Type>(rawSection);

    switch (section) {
    case FlatpakPermissionsSectionType::Filesystems:
        return QStringList{i18n("read/write"), i18n("read-only"), i18n("create")};
    case FlatpakPermissionsSectionType::SessionBus:
    case FlatpakPermissionsSectionType::SystemBus:
        return QStringList{i18n("None"), i18n("talk"), i18n("own"), i18n("see")};
    case FlatpakPermissionsSectionType::Basic:
    case FlatpakPermissionsSectionType::Advanced:
    case FlatpakPermissionsSectionType::SubsystemsShared:
    case FlatpakPermissionsSectionType::Sockets:
    case FlatpakPermissionsSectionType::Devices:
    case FlatpakPermissionsSectionType::Features:
    case FlatpakPermissionsSectionType::Environment:
        break;
    }

    return {};
}

Q_INVOKABLE QString FlatpakPermissionModel::sectionHeaderForSectionType(int /*FlatpakPermissionsSectionType::Type*/ rawSection)
{
    if (!QMetaEnum::fromType<FlatpakPermissionsSectionType::Type>().valueToKey(rawSection)) {
        return {};
    }
    // SAFETY: QMetaEnum above ensures that coercion is valid.
    const auto section = static_cast<FlatpakPermissionsSectionType::Type>(rawSection);

    switch (section) {
    case FlatpakPermissionsSectionType::Basic:
        return i18n("Basic Permissions");
    case FlatpakPermissionsSectionType::Filesystems:
        return i18n("Filesystem Access");
    case FlatpakPermissionsSectionType::Advanced:
        return i18n("Advanced Permissions");
    case FlatpakPermissionsSectionType::SubsystemsShared:
        return i18n("Subsystems Shared");
    case FlatpakPermissionsSectionType::Sockets:
        return i18n("Sockets");
    case FlatpakPermissionsSectionType::Devices:
        return i18n("Device Access");
    case FlatpakPermissionsSectionType::Features:
        return i18n("Features Allowed");
    case FlatpakPermissionsSectionType::SessionBus:
        return i18n("Session Bus Policy");
    case FlatpakPermissionsSectionType::SystemBus:
        return i18n("System Bus Policy");
    case FlatpakPermissionsSectionType::Environment:
        return i18n("Environment");
    }

    Q_UNREACHABLE();
    return {};
}

Q_INVOKABLE QString FlatpakPermissionModel::sectionAddButtonToolTipTextForSectionType(int /*FlatpakPermissionsSectionType::Type*/ rawSection)
{
    if (!QMetaEnum::fromType<FlatpakPermissionsSectionType::Type>().valueToKey(rawSection)) {
        return {};
    }
    // SAFETY: QMetaEnum above ensures that coercion is valid.
    const auto section = static_cast<FlatpakPermissionsSectionType::Type>(rawSection);

    switch (section) {
    case FlatpakPermissionsSectionType::Filesystems:
        return i18n("Add a new filesystem path");
    case FlatpakPermissionsSectionType::SessionBus:
        return i18n("Add a new session bus");
    case FlatpakPermissionsSectionType::SystemBus:
        return i18n("Add a new system bus");
    case FlatpakPermissionsSectionType::Environment:
        return i18n("Add a new environment variable");
    case FlatpakPermissionsSectionType::Basic:
    case FlatpakPermissionsSectionType::Advanced:
    case FlatpakPermissionsSectionType::SubsystemsShared:
    case FlatpakPermissionsSectionType::Sockets:
    case FlatpakPermissionsSectionType::Devices:
    case FlatpakPermissionsSectionType::Features:
        break;
    }

    return {};
}

void FlatpakPermissionModel::togglePermissionAtIndex(int index)
{
    /* guard for invalid indices */
    if (index < 0 || index >= m_permissions.length()) {
        return;
    }

    FlatpakPermission *perm = &m_permissions[index];
    bool wasEnabled = perm->isEffectiveEnabled();

    if (perm->valueType() == FlatpakPermission::ValueType::Simple) {
        bool setToNonDefault = (perm->isDefaultEnabled() && wasEnabled) || (!perm->isDefaultEnabled() && !wasEnabled);
        if (setToNonDefault) {
            addPermission(perm, !wasEnabled);
        } else {
            removePermission(perm, wasEnabled);
        }
        perm->setEffectiveEnabled(!perm->isEffectiveEnabled());
    } else if (perm->valueType() == FlatpakPermission::ValueType::Filesystems) {
        if (perm->isDefaultEnabled() && wasEnabled) {
            /* if access level ("value") was changed, there's already an entry. Prepend ! to it.
             * if access level is unchanged, add a new entry */
            if (perm->defaultValue() != perm->effectiveValue()) {
                int permIndex = m_overridesData.indexOf(perm->name());
                m_overridesData.insert(permIndex, QLatin1Char('!'));
            } else {
                addPermission(perm, !wasEnabled);
            }
        } else if (!perm->isDefaultEnabled() && !wasEnabled) {
            if (!m_overridesData.contains(perm->name())) {
                addPermission(perm, !wasEnabled);
            }
            if (perm->originType() == FlatpakPermission::OriginType::UserDefined) {
                m_overridesData.remove(m_overridesData.indexOf(perm->name()) - 1, 1);
            }
            // set value to read/write automatically, if not done already
        } else if (perm->isDefaultEnabled() && !wasEnabled) {
            /* if access level ("value") was changed, just remove ! from beginning.
             * if access level is unchanged, remove the whole entry */
            if (perm->defaultValue() != perm->effectiveValue()) {
                int permIndex = m_overridesData.indexOf(perm->name());
                m_overridesData.remove(permIndex - 1, 1);
            } else {
                removePermission(perm, wasEnabled);
            }
        } else if (!perm->isDefaultEnabled() && wasEnabled) {
            removePermission(perm, wasEnabled);
        }
        perm->setEffectiveEnabled(!perm->isEffectiveEnabled());
    } else if (perm->valueType() == FlatpakPermission::ValueType::Bus) {
        if (perm->isDefaultEnabled()) {
            qWarning() << "Illegal operation: D-Bus value provided by upstream can not be toggled:" << perm->name();
        } else if (!perm->isDefaultEnabled() && wasEnabled) {
            perm->setEffectiveEnabled(false);
            removeBusPermission(perm);
        } else if (!perm->isDefaultEnabled() && !wasEnabled) {
            perm->setEffectiveEnabled(true);
            addBusPermissions(perm);
        }
    } else if (perm->valueType() == FlatpakPermission::ValueType::Environment) {
        if (perm->isDefaultEnabled() && wasEnabled) {
            perm->setEffectiveEnabled(false);
            if (perm->defaultValue() != perm->effectiveValue()) {
                editEnvPermission(perm, QString());
            } else {
                addEnvPermission(perm);
            }
        } else if (perm->isDefaultEnabled() && !wasEnabled) {
            if (perm->defaultValue() != perm->effectiveValue()) {
                editEnvPermission(perm, perm->effectiveValue());
            } else {
                removeEnvPermission(perm);
            }
            perm->setEffectiveEnabled(true);
        } else if (!perm->isDefaultEnabled() && wasEnabled) {
            perm->setEffectiveEnabled(false);
            removeEnvPermission(perm);
        } else if (!perm->isDefaultEnabled() && !wasEnabled) {
            perm->setEffectiveEnabled(true);
            addEnvPermission(perm);
        }
    }

    const auto idx = FlatpakPermissionModel::index(index, 0);
    Q_EMIT dataChanged(idx, idx);
}

void FlatpakPermissionModel::editPerm(int index, const QString &newValue)
{
    /* guard for out-of-range indices */
    if (index < 0 || index >= m_permissions.length()) {
        return;
    }

    FlatpakPermission *perm = &m_permissions[index];

    switch (perm->section()) {
    case FlatpakPermissionsSectionType::Filesystems:
        editFilesystemsPermissions(perm, newValue);
        break;
    case FlatpakPermissionsSectionType::SessionBus:
    case FlatpakPermissionsSectionType::SystemBus:
        editBusPermissions(perm, newValue);
        break;
    case FlatpakPermissionsSectionType::Environment:
        editEnvPermission(perm, newValue);
        break;
    case FlatpakPermissionsSectionType::Basic:
    case FlatpakPermissionsSectionType::Advanced:
    case FlatpakPermissionsSectionType::SubsystemsShared:
    case FlatpakPermissionsSectionType::Sockets:
    case FlatpakPermissionsSectionType::Devices:
    case FlatpakPermissionsSectionType::Features:
        return;
    }

    const auto idx = FlatpakPermissionModel::index(index, 0);
    Q_EMIT dataChanged(idx, idx);
}

void FlatpakPermissionModel::addUserEnteredPermission(int /*FlatpakPermissionsSectionType::Type*/ rawSection, const QString &name, const QString &value)
{
    if (!QMetaEnum::fromType<FlatpakPermissionsSectionType::Type>().valueToKey(rawSection)) {
        return;
    }
    // SAFETY: QMetaEnum above ensures that coercion is valid.
    const auto section = static_cast<FlatpakPermissionsSectionType::Type>(rawSection);
    QString category;

    switch (section) {
    case FlatpakPermissionsSectionType::Filesystems:
        category = QLatin1String(FLATPAK_METADATA_KEY_FILESYSTEMS);
        break;
    case FlatpakPermissionsSectionType::SessionBus:
        category = QLatin1String(FLATPAK_METADATA_GROUP_SESSION_BUS_POLICY);
        break;
    case FlatpakPermissionsSectionType::SystemBus:
        category = QLatin1String(FLATPAK_METADATA_GROUP_SYSTEM_BUS_POLICY);
        break;
    case FlatpakPermissionsSectionType::Environment:
        category = QLatin1String(FLATPAK_METADATA_GROUP_ENVIRONMENT);
        break;
    case FlatpakPermissionsSectionType::Basic:
    case FlatpakPermissionsSectionType::Advanced:
    case FlatpakPermissionsSectionType::SubsystemsShared:
    case FlatpakPermissionsSectionType::Sockets:
    case FlatpakPermissionsSectionType::Devices:
    case FlatpakPermissionsSectionType::Features:
        return;
    }

    const auto possibleValues = valueListForSectionType(rawSection);

    FlatpakPermission perm(section, name, category, name, false, QString(), possibleValues);
    perm.setOriginType(FlatpakPermission::OriginType::UserDefined);
    perm.setEffectiveEnabled(true);
    perm.setEffectiveValue(value);

    int index = permIndex(category);
    m_permissions.insert(index, perm);

    switch (section) {
    case FlatpakPermissionsSectionType::Filesystems:
        addPermission(&perm, true);
        break;
    case FlatpakPermissionsSectionType::SessionBus:
    case FlatpakPermissionsSectionType::SystemBus:
        addBusPermissions(&perm);
        break;
    case FlatpakPermissionsSectionType::Environment:
        addEnvPermission(&perm);
        break;
    case FlatpakPermissionsSectionType::Basic:
    case FlatpakPermissionsSectionType::Advanced:
    case FlatpakPermissionsSectionType::SubsystemsShared:
    case FlatpakPermissionsSectionType::Sockets:
    case FlatpakPermissionsSectionType::Devices:
    case FlatpakPermissionsSectionType::Features:
        return;
    }

    const auto idx = FlatpakPermissionModel::index(index, 0);
    Q_EMIT dataChanged(idx, idx);
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
    if (perm->valueType() == FlatpakPermission::ValueType::Filesystems) {
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

    if (perm->valueType() == FlatpakPermission::ValueType::Filesystems) {
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
    QString val = perm->effectiveValue();
    m_overridesData.insert(permIndex, perm->name() + QLatin1Char('=') + val + QLatin1Char('\n'));
}

void FlatpakPermissionModel::removeBusPermission(FlatpakPermission *perm)
{
    int permBeginIndex = m_overridesData.indexOf(perm->name() + QLatin1Char('='));
    if (permBeginIndex == -1) {
        return;
    }

    int permLen = perm->name().length() + 1 + perm->effectiveValue().length() + 1;
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

    if (perm->isDefaultEnabled() == perm->isEffectiveEnabled() && perm->defaultValue() == newValue) {
        removePermission(perm, true);
        perm->setEffectiveValue(newValue);
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
    perm->setEffectiveValue(newValue);
}

void FlatpakPermissionModel::editBusPermissions(FlatpakPermission *perm, const QString &l10nValue)
{
    if (perm->isDefaultEnabled() && l10nValue == perm->overrideValue()) {
        perm->setEffectiveValue(l10nValue);
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
    perm->setEffectiveValue(l10nValue);
}

void FlatpakPermissionModel::addEnvPermission(FlatpakPermission *perm)
{
    QString groupHeader = QLatin1Char('[') + perm->category() + QLatin1Char(']');
    if (!m_overridesData.contains(groupHeader)) {
        m_overridesData.insert(m_overridesData.length(), groupHeader + QLatin1Char('\n'));
    }
    int permIndex = m_overridesData.indexOf(QLatin1Char('\n'), m_overridesData.indexOf(groupHeader)) + 1;
    QString val = perm->isEffectiveEnabled() ? perm->effectiveValue() : QString();
    m_overridesData.insert(permIndex, perm->name() + QLatin1Char('=') + val + QLatin1Char('\n'));
}

void FlatpakPermissionModel::removeEnvPermission(FlatpakPermission *perm)
{
    int permBeginIndex = m_overridesData.indexOf(perm->name());
    if (permBeginIndex == -1) {
        return;
    }

    int valueLen = perm->isEffectiveEnabled() ? perm->effectiveValue().length() + 1 : 0;

    int permLen = perm->name().length() + 1 + valueLen;
    m_overridesData.remove(permBeginIndex, permLen);
}

void FlatpakPermissionModel::editEnvPermission(FlatpakPermission *perm, const QString &value)
{
    if (perm->isDefaultEnabled() && value == perm->defaultValue()) {
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
        perm->setEffectiveValue(value);
    }
}

bool FlatpakPermissionModel::permExists(const QString &name) const
{
    return std::any_of(m_permissions.cbegin(), m_permissions.cend(), [name](const FlatpakPermission &permission) {
        return permission.name() == name;
    });
}

int FlatpakPermissionModel::permIndex(const QString &category)
{
    int i = 0;
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
    if (m_permissions.at(i - 1).originType() == FlatpakPermission::OriginType::Dummy) {
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

    QFile inFile(m_reference->path());
    if (!inFile.open(QIODevice::ReadOnly)) {
        return;
    }
    QTextStream inStream(&inFile);
    m_overridesData = inStream.readAll();
    inFile.close();
}

void FlatpakPermissionModel::writeToFile()
{
    QFile outFile(m_reference->path());
    if (!outFile.open(QIODevice::WriteOnly)) {
        qInfo() << "File does not open for write only";
    }
    QTextStream outStream(&outFile);
    outStream << m_overridesData;
    outFile.close();
}
