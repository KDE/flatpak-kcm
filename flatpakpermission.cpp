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
#include <QChar>
#include <QDebug>
#include <QFileInfo>
#include <QMetaEnum>
#include <QQmlEngine>
#include <QTemporaryFile>
#include <QUrl>

#include <algorithm>
#include <array>
#include <optional>
#include <variant>

namespace
{

const QLatin1String SUFFIX_RO = QLatin1String(":ro");
const QLatin1String SUFFIX_RW = QLatin1String(":rw");
const QLatin1String SUFFIX_CREATE = QLatin1String(":create");
const QLatin1Char PREFIX_DENY = QLatin1Char('!');

using FilesystemPrefix = FlatpakFilesystemsEntry::FilesystemPrefix;
using PathMode = FlatpakFilesystemsEntry::PathMode;

constexpr FlatpakFilesystemsEntry::TableEntry makeRequiredPath(FilesystemPrefix prefix, const char *prefixString)
{
    return FlatpakFilesystemsEntry::TableEntry{prefix, PathMode::Required, QLatin1String(), QLatin1String(prefixString)};
}

constexpr FlatpakFilesystemsEntry::TableEntry makeOptionalPath(FilesystemPrefix prefix, const char *fixedString, const char *prefixString)
{
    return FlatpakFilesystemsEntry::TableEntry{prefix, PathMode::Optional, QLatin1String(fixedString), QLatin1String(prefixString)};
}

constexpr FlatpakFilesystemsEntry::TableEntry makeInvalidPath(FilesystemPrefix prefix, const char *fixedString)
{
    return FlatpakFilesystemsEntry::TableEntry{prefix, PathMode::NoPath, QLatin1String(fixedString), QLatin1String()};
}

const auto s_filesystems = {
    //
    makeRequiredPath(FilesystemPrefix::Absolute, "/"),
    //
    makeOptionalPath(FilesystemPrefix::Home, "~", "~/"),
    makeOptionalPath(FilesystemPrefix::Home, "home", "home/"),
    //
    makeInvalidPath(FilesystemPrefix::Host, "host"),
    makeInvalidPath(FilesystemPrefix::HostOs, "host-os"),
    makeInvalidPath(FilesystemPrefix::HostEtc, "host-etc"),
    //
    makeOptionalPath(FilesystemPrefix::XdgDesktop, "xdg-desktop", "xdg-desktop/"),
    makeOptionalPath(FilesystemPrefix::XdgDocuments, "xdg-documents", "xdg-documents/"),
    makeOptionalPath(FilesystemPrefix::XdgDownload, "xdg-download", "xdg-download/"),
    makeOptionalPath(FilesystemPrefix::XdgMusic, "xdg-music", "xdg-music/"),
    makeOptionalPath(FilesystemPrefix::XdgPictures, "xdg-pictures", "xdg-pictures/"),
    makeOptionalPath(FilesystemPrefix::XdgPublicShare, "xdg-public-share", "xdg-public-share/"),
    makeOptionalPath(FilesystemPrefix::XdgVideos, "xdg-videos", "xdg-videos/"),
    makeOptionalPath(FilesystemPrefix::XdgTemplates, "xdg-templates", "xdg-templates/"),
    //
    makeOptionalPath(FilesystemPrefix::XdgCache, "xdg-cache", "xdg-cache/"),
    makeOptionalPath(FilesystemPrefix::XdgConfig, "xdg-config", "xdg-config/"),
    makeOptionalPath(FilesystemPrefix::XdgData, "xdg-data", "xdg-data/"),
    //
    makeRequiredPath(FilesystemPrefix::XdgRun, "xdg-run/"),
    //
    makeRequiredPath(FilesystemPrefix::Unknown, ""),
};

} // namespace

FlatpakFilesystemsEntry::FlatpakFilesystemsEntry(FilesystemPrefix prefix, AccessMode mode, const QString &path)
    : m_prefix(prefix)
    , m_mode(mode)
    , m_path(path)
{
}

std::optional<FlatpakFilesystemsEntry> FlatpakFilesystemsEntry::parse(QStringView entry)
{
    std::optional<AccessMode> accessMode = std::nullopt;

    if (entry.endsWith(SUFFIX_RO)) {
        entry.chop(SUFFIX_RO.size());
        accessMode = std::optional(AccessMode::ReadOnly);
    } else if (entry.endsWith(SUFFIX_RW)) {
        entry.chop(SUFFIX_RW.size());
        accessMode = std::optional(AccessMode::ReadWrite);
    } else if (entry.endsWith(SUFFIX_CREATE)) {
        entry.chop(SUFFIX_CREATE.size());
        accessMode = std::optional(AccessMode::Create);
    }

    if (entry.startsWith(PREFIX_DENY)) {
        // ensure there is no access mode suffix
        if (accessMode.has_value()) {
            return std::nullopt;
        }
        entry = entry.mid(1);
        accessMode = std::optional(AccessMode::Deny);
    }

    AccessMode effectiveAccessMode = accessMode.value_or(AccessMode::ReadWrite);

    for (const TableEntry &filesystem : s_filesystems) {
        // Deliberately not using switch here, because of the overlapping Optional case.
        if (filesystem.mode == PathMode::Optional || filesystem.mode == PathMode::Required) {
            if (entry.startsWith(filesystem.prefixString) || filesystem.prefix == FilesystemPrefix::Unknown) {
                if (filesystem.prefix != FilesystemPrefix::Unknown) {
                    entry = entry.mid(filesystem.prefixString.size());
                }
                QString path;
                if (!entry.isEmpty()) {
                    path = QUrl(entry.toString()).toDisplayString(QUrl::RemoveScheme | QUrl::StripTrailingSlash);
                } else if (filesystem.mode == PathMode::Required) {
                    return std::nullopt;
                }
                return std::optional(FlatpakFilesystemsEntry(filesystem.prefix, effectiveAccessMode, path));
            }
        }
        if (filesystem.mode == PathMode::NoPath || filesystem.mode == PathMode::Optional) {
            if (entry == filesystem.fixedString) {
                return std::optional(FlatpakFilesystemsEntry(filesystem.prefix, effectiveAccessMode, QString()));
            }
        }
    }

    return std::nullopt;
}

QString FlatpakFilesystemsEntry::format() const
{
    const auto it = std::find_if(s_filesystems.begin(), s_filesystems.end(), [this](const TableEntry &filesystem) {
        if (filesystem.prefix != m_prefix) {
            return false;
        }
        // home/path should be serialized as ~/path, and fixed string "~" as "home".
        // So either the path should be empty, XOR current table entry is the "~" variant.
        if (filesystem.prefix == FilesystemPrefix::Home) {
            return m_path.isEmpty() != (filesystem.fixedString == QLatin1String("~"));
        }
        return true;
    });
    if (it == s_filesystems.end()) {
        // all prefixes are covered in the table, so this should never happen.
        Q_UNREACHABLE();
        return {};
    }

    if ((m_path.isEmpty() && it->mode == PathMode::Required) || (!m_path.isEmpty() && it->mode == PathMode::NoPath)) {
        return {};
    }

    const QString path = (m_path.isEmpty() ? QString(it->fixedString) : it->prefixString + m_path);

    switch (m_mode) {
    case AccessMode::ReadOnly:
        return path + SUFFIX_RO;
    case AccessMode::ReadWrite:
        // Omit default value
        return path;
    case AccessMode::Create:
        return path + SUFFIX_CREATE;
    case AccessMode::Deny:
        return PREFIX_DENY + path;
    }
    return {};
}

FlatpakFilesystemsEntry::FilesystemPrefix FlatpakFilesystemsEntry::prefix() const
{
    return m_prefix;
}

QString FlatpakFilesystemsEntry::path() const
{
    return m_path;
}

FlatpakFilesystemsEntry::AccessMode FlatpakFilesystemsEntry::mode() const
{
    return m_mode;
}

bool FlatpakFilesystemsEntry::operator==(const FlatpakFilesystemsEntry &other) const
{
    return other.m_prefix == m_prefix && other.m_mode == m_mode && other.m_path == m_path;
}

bool FlatpakFilesystemsEntry::operator!=(const FlatpakFilesystemsEntry &other) const
{
    return !(*this == other);
}

PolicyChoicesModel::PolicyChoicesModel(QVector<Entry> &&policies, QObject *parent)
    : QAbstractListModel(parent)
    , m_policies(policies)
{
}

QHash<int, QByteArray> PolicyChoicesModel::roleNames() const
{
    return {
        {Qt::DisplayRole, "display"},
        {Roles::ValueRole, "value"},
    };
}

int PolicyChoicesModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return m_policies.count();
}

QVariant PolicyChoicesModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_policies.count()) {
        return {};
    }

    const auto policy = m_policies.at(index.row());

    switch (role) {
    case Qt::DisplayRole:
        return policy.display;
    case Roles::ValueRole:
        return policy.value;
    }

    return {};
}

FilesystemChoicesModel::FilesystemChoicesModel(QObject *parent)
    : PolicyChoicesModel(
        QVector<Entry>{
            {static_cast<int>(FlatpakFilesystemsEntry::AccessMode::ReadOnly), i18n("read-only")},
            {static_cast<int>(FlatpakFilesystemsEntry::AccessMode::ReadWrite), i18n("read/write")},
            {static_cast<int>(FlatpakFilesystemsEntry::AccessMode::Create), i18n("create")},
            // TODO: Model logic is not ready to process this value yet.
            /* {static_cast<int>(FlatpakFilesystemsEntry::AccessMode::Deny), i18n("OFF")}, */
        },
        parent)
{
}

DBusPolicyChoicesModel::DBusPolicyChoicesModel(QObject *parent)
    : PolicyChoicesModel(
        QVector<Entry>{
            {FlatpakPolicy::FLATPAK_POLICY_NONE, i18n("None")},
            {FlatpakPolicy::FLATPAK_POLICY_SEE, i18n("see")},
            {FlatpakPolicy::FLATPAK_POLICY_TALK, i18n("talk")},
            {FlatpakPolicy::FLATPAK_POLICY_OWN, i18n("own")},
        },
        parent)
{
}

Q_GLOBAL_STATIC(FilesystemChoicesModel, s_FilesystemPolicies);
Q_GLOBAL_STATIC(DBusPolicyChoicesModel, s_DBusPolicies);

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
                                     const Variant &defaultValue)
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

const FlatpakPermission::Variant FlatpakPermission::defaultValue() const
{
    return m_defaultValue;
}

const FlatpakPermission::Variant FlatpakPermission::overrideValue() const
{
    return m_overrideValue;
}

void FlatpakPermission::setOverrideValue(const Variant &value)
{
    m_overrideValue = value;
}

const FlatpakPermission::Variant FlatpakPermission::effectiveValue() const
{
    return m_effectiveValue;
}

void FlatpakPermission::setEffectiveValue(const Variant &value)
{
    m_effectiveValue = value;
}

QString FlatpakPermission::fsCurrentValue() const
{
    // NB: the use of i18n here is actually kind of wrong but at the time of writing fixing the mapping
    // between ui and backend *here* is easier/safer than trying to reinvent the way the mapping works in a
    // more reliable fashion.
    if (!std::holds_alternative<QString>(m_effectiveValue)) {
        qWarning() << "Expected string, got alternative index" << m_effectiveValue.index();
        return {};
    }
    const auto value = std::get<QString>(m_effectiveValue);
    if (value == i18n("OFF")) {
        return QString();
    }
    if (value == i18n("read-only")) {
        return QLatin1String("ro");
    }
    if (value == i18n("create")) {
        return QLatin1String("create");
    }
    return QLatin1String("rw");
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

static QString mapDBusFlatpakPolicyEnumValueToConfigString(FlatpakPolicy value)
{
    switch (value) {
    case FlatpakPolicy::FLATPAK_POLICY_SEE:
        return QStringLiteral("see");
    case FlatpakPolicy::FLATPAK_POLICY_TALK:
        return QStringLiteral("talk");
    case FlatpakPolicy::FLATPAK_POLICY_OWN:
        return QStringLiteral("own");
    case FlatpakPolicy::FLATPAK_POLICY_NONE:
        break;
    }
    return QStringLiteral("none");
}

static FlatpakPolicy mapDBusFlatpakPolicyConfigStringToEnumValue(const QString &value)
{
    if (value == QStringLiteral("see")) {
        return FlatpakPolicy::FLATPAK_POLICY_SEE;
    }
    if (value == QStringLiteral("talk")) {
        return FlatpakPolicy::FLATPAK_POLICY_TALK;
    }
    if (value == QStringLiteral("own")) {
        return FlatpakPolicy::FLATPAK_POLICY_OWN;
    }
    if (value != QStringLiteral("none")) {
        qWarning() << "Unsupported Flatpak D-Bus policy:" << value;
    }
    return FlatpakPolicy::FLATPAK_POLICY_NONE;
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
        return QVariant::fromStdVariant(permission.defaultValue());
    case Roles::EffectiveValue:
        return QVariant::fromStdVariant(permission.effectiveValue());
    //
    case Roles::ValuesModel:
        return QVariant::fromValue(FlatpakPermissionModel::valuesModelForSectionType(permission.section()));
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
    roles[Roles::ValuesModel] = "valuesModel";
    return roles;
}

void FlatpakPermissionModel::loadDefaultValues()
{
    QString name;
    QString category;
    QString description;
    QString defaultValue;
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
            filesysTemp.append(FlatpakPermission(FlatpakPermissionsSectionType::Filesystems, name, category, description, isEnabledByDefault, fileSystemValue));
        }
    }

    name = QStringLiteral("home");
    description = i18n("All User Files");
    if (homeVal == i18n("OFF")) {
        isEnabledByDefault = false;
        homeVal = i18n("read/write");
    } else {
        isEnabledByDefault = true;
    }
    m_permissions.insert(basicIndex, FlatpakPermission(FlatpakPermissionsSectionType::Filesystems, name, category, description, isEnabledByDefault, homeVal));
    basicIndex += 1;

    name = QStringLiteral("host");
    description = i18n("All System Files");
    if (hostVal == i18n("OFF")) {
        isEnabledByDefault = false;
        hostVal = i18n("read/write");
    } else {
        isEnabledByDefault = true;
    }
    m_permissions.insert(basicIndex, FlatpakPermission(FlatpakPermissionsSectionType::Filesystems, name, category, description, isEnabledByDefault, hostVal));
    basicIndex += 1;

    name = QStringLiteral("host-os");
    description = i18n("All System Libraries, Executables and Binaries");
    if (hostOsVal == i18n("OFF")) {
        isEnabledByDefault = false;
        hostOsVal = i18n("read/write");
    } else {
        isEnabledByDefault = true;
    }
    m_permissions.insert(basicIndex, FlatpakPermission(FlatpakPermissionsSectionType::Filesystems, name, category, description, isEnabledByDefault, hostOsVal));
    basicIndex += 1;

    name = QStringLiteral("host-etc");
    description = i18n("All System Configurations");
    if (hostEtcVal == i18n("OFF")) {
        isEnabledByDefault = false;
        hostEtcVal = i18n("read/write");
    } else {
        isEnabledByDefault = true;
    }
    m_permissions.insert(basicIndex,
                         FlatpakPermission(FlatpakPermissionsSectionType::Filesystems, name, category, description, isEnabledByDefault, hostEtcVal));
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
    if (sessionBusGroup.exists() && !sessionBusGroup.entryMap().isEmpty()) {
        const auto entries = sessionBusGroup.entryMap();
        const auto keys = entries.keys();
        for (int i = 0; i < keys.length(); ++i) {
            name = description = keys.at(i);
            const auto policyValue = mapDBusFlatpakPolicyConfigStringToEnumValue(entries.value(keys.at(i)));
            isEnabledByDefault = true;
            m_permissions.append(FlatpakPermission(FlatpakPermissionsSectionType::SessionBus, name, category, description, isEnabledByDefault, policyValue));
        }
    } else {
        m_permissions.append(FlatpakPermission(FlatpakPermissionsSectionType::SessionBus));
    }
    /* SESSION BUS category */

    /* SYSTEM BUS category */
    category = QLatin1String(FLATPAK_METADATA_GROUP_SYSTEM_BUS_POLICY);
    const KConfigGroup systemBusGroup = parser.group(QLatin1String(FLATPAK_METADATA_GROUP_SYSTEM_BUS_POLICY));
    if (systemBusGroup.exists() && !systemBusGroup.entryMap().isEmpty()) {
        const auto entries = systemBusGroup.entryMap();
        const auto keys = entries.keys();
        for (int i = 0; i < keys.length(); ++i) {
            name = description = keys.at(i);
            const auto policyValue = mapDBusFlatpakPolicyConfigStringToEnumValue(entries.value(keys.at(i)));
            isEnabledByDefault = true;
            m_permissions.append(FlatpakPermission(FlatpakPermissionsSectionType::SystemBus, name, category, description, isEnabledByDefault, policyValue));
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
        if (environmentGroup.exists() && !environmentGroup.entryMap().isEmpty()) {
            const auto entries = environmentGroup.entryMap();
            const auto keys = entries.keys();
            for (int i = 0; i < keys.length(); ++i) {
                name = description = keys.at(i);
                defaultValue = entries.value(keys.at(i));
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
    const QString path = m_reference->permissionsFilename();

    /* all permissions are at default, so nothing to load */
    if (!QFileInfo::exists(path)) {
        return;
    }

    KConfig parser(path);
    const KConfigGroup contextGroup = parser.group(FLATPAK_METADATA_GROUP_CONTEXT);

    int fsIndex = -1;

    for (int i = 0; i < m_permissions.length(); ++i) {
        FlatpakPermission &permission = m_permissions[i];

        switch (permission.valueType()) {
        case FlatpakPermission::ValueType::Simple: {
            const QString cat = contextGroup.readEntry(permission.category(), QString());
            if (cat.contains(permission.name())) {
                // permission.setEnabled(!permission.enabledByDefault());
                bool isEnabled = !permission.isDefaultEnabled();
                permission.setEffectiveEnabled(isEnabled);
                permission.setOverrideEnabled(isEnabled);
            }
            break;
        }
        case FlatpakPermission::ValueType::Filesystems: {
            const QString cat = contextGroup.readEntry(permission.category(), QString());
            if (cat.contains(permission.name())) {
                int permIndex = cat.indexOf(permission.name());

                /* the permission is just being set off, the access level isn't changed */
                bool isEnabled = permIndex > 0 ? cat.at(permIndex - 1) != QLatin1Char('!') : true;
                permission.setEffectiveEnabled(isEnabled);
                permission.setOverrideEnabled(isEnabled);

                int valueIndex = permIndex + permission.name().length();
                QString val;

                if (valueIndex >= cat.length() || cat.at(valueIndex) != QLatin1Char(':')) {
                    val = i18n("read/write");
                    permission.setEffectiveValue(val);
                    permission.setOverrideValue(val);
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
                permission.setEffectiveValue(val);
                permission.setOverrideValue(val);
            }
            fsIndex = i + 1;
            break;
        }
        case FlatpakPermission::ValueType::Bus:
        case FlatpakPermission::ValueType::Environment: {
            const KConfigGroup group = parser.group(permission.category());
            if (group.exists()) {
                QMap<QString, QString> map = group.entryMap();
                QList<QString> list = map.keys();
                int index = list.indexOf(permission.name());
                if (index != -1) {
                    QString value = map.value(permission.name());
                    if (permission.valueType() == FlatpakPermission::ValueType::Bus) {
                        const auto policyValue = mapDBusFlatpakPolicyConfigStringToEnumValue(value);
                        permission.setOverrideValue(policyValue);
                        permission.setEffectiveValue(policyValue);
                    } else {
                        permission.setOverrideValue(value);
                        permission.setEffectiveValue(value);
                    }

                    permission.setOverrideEnabled(true);
                    permission.setEffectiveEnabled(true);
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
                m_permissions.insert(
                    fsIndex,
                    FlatpakPermission(FlatpakPermissionsSectionType::Filesystems, name, QLatin1String(FLATPAK_METADATA_KEY_FILESYSTEMS), name, false, value));
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
                    const auto policyValue = mapDBusFlatpakPolicyConfigStringToEnumValue(value);
                    m_permissions.insert(insertIndex, FlatpakPermission(section, name, category, name, false, policyValue));
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

PolicyChoicesModel *FlatpakPermissionModel::valuesModelForSectionType(int /*FlatpakPermissionsSectionType::Type*/ rawSection)
{
    if (!QMetaEnum::fromType<FlatpakPermissionsSectionType::Type>().valueToKey(rawSection)) {
        return {};
    }
    // SAFETY: QMetaEnum above ensures that coercion is valid.
    const auto section = static_cast<FlatpakPermissionsSectionType::Type>(rawSection);

    switch (section) {
    case FlatpakPermissionsSectionType::Filesystems:
        return valuesModelForFilesystemsSection();
    case FlatpakPermissionsSectionType::SessionBus:
    case FlatpakPermissionsSectionType::SystemBus:
        return valuesModelForBusSections();
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

PolicyChoicesModel *FlatpakPermissionModel::valuesModelForFilesystemsSection()
{
    QQmlEngine::setObjectOwnership(s_FilesystemPolicies, QQmlEngine::CppOwnership);
    return s_FilesystemPolicies;
}

PolicyChoicesModel *FlatpakPermissionModel::valuesModelForBusSections()
{
    QQmlEngine::setObjectOwnership(s_DBusPolicies, QQmlEngine::CppOwnership);
    return s_DBusPolicies;
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

    FlatpakPermission &permission = m_permissions[index];
    bool wasEnabled = permission.isEffectiveEnabled();

    if (permission.valueType() == FlatpakPermission::ValueType::Simple) {
        bool setToNonDefault = (permission.isDefaultEnabled() && wasEnabled) || (!permission.isDefaultEnabled() && !wasEnabled);
        if (setToNonDefault) {
            addPermission(permission, !wasEnabled);
        } else {
            removePermission(permission, wasEnabled);
        }
        permission.setEffectiveEnabled(!permission.isEffectiveEnabled());
    } else if (permission.valueType() == FlatpakPermission::ValueType::Filesystems) {
        if (permission.isDefaultEnabled() && wasEnabled) {
            /* if access level ("value") was changed, there's already an entry. Prepend ! to it.
             * if access level is unchanged, add a new entry */
            if (permission.defaultValue() != permission.effectiveValue()) {
                int permIndex = m_overridesData.indexOf(permission.name());
                m_overridesData.insert(permIndex, QLatin1Char('!'));
            } else {
                addPermission(permission, !wasEnabled);
            }
        } else if (!permission.isDefaultEnabled() && !wasEnabled) {
            if (!m_overridesData.contains(permission.name())) {
                addPermission(permission, !wasEnabled);
            }
            if (permission.originType() == FlatpakPermission::OriginType::UserDefined) {
                m_overridesData.remove(m_overridesData.indexOf(permission.name()) - 1, 1);
            }
            // set value to read/write automatically, if not done already
        } else if (permission.isDefaultEnabled() && !wasEnabled) {
            /* if access level ("value") was changed, just remove ! from beginning.
             * if access level is unchanged, remove the whole entry */
            if (permission.defaultValue() != permission.effectiveValue()) {
                int permIndex = m_overridesData.indexOf(permission.name());
                m_overridesData.remove(permIndex - 1, 1);
            } else {
                removePermission(permission, wasEnabled);
            }
        } else if (!permission.isDefaultEnabled() && wasEnabled) {
            removePermission(permission, wasEnabled);
        }
        permission.setEffectiveEnabled(!permission.isEffectiveEnabled());
    } else if (permission.valueType() == FlatpakPermission::ValueType::Bus) {
        if (permission.isDefaultEnabled()) {
            qWarning() << "Illegal operation: D-Bus value provided by upstream can not be toggled:" << permission.name();
        } else if (!permission.isDefaultEnabled() && wasEnabled) {
            permission.setEffectiveEnabled(false);
            removeBusPermission(permission);
        } else if (!permission.isDefaultEnabled() && !wasEnabled) {
            permission.setEffectiveEnabled(true);
            addBusPermissions(permission);
        }
    } else if (permission.valueType() == FlatpakPermission::ValueType::Environment) {
        if (permission.isDefaultEnabled() && wasEnabled) {
            permission.setEffectiveEnabled(false);
            if (permission.defaultValue() != permission.effectiveValue()) {
                editEnvPermission(permission, QString());
            } else {
                addEnvPermission(permission);
            }
        } else if (permission.isDefaultEnabled() && !wasEnabled) {
            if (permission.defaultValue() != permission.effectiveValue()) {
                editEnvPermission(permission, std::get<QString>(permission.effectiveValue()));
            } else {
                removeEnvPermission(permission);
            }
            permission.setEffectiveEnabled(true);
        } else if (!permission.isDefaultEnabled() && wasEnabled) {
            permission.setEffectiveEnabled(false);
            removeEnvPermission(permission);
        } else if (!permission.isDefaultEnabled() && !wasEnabled) {
            permission.setEffectiveEnabled(true);
            addEnvPermission(permission);
        }
    }

    const auto idx = FlatpakPermissionModel::index(index, 0);
    Q_EMIT dataChanged(idx, idx);
}

void FlatpakPermissionModel::editPerm(int index, const QVariant &newValue)
{
    /* guard for out-of-range indices */
    if (index < 0 || index >= m_permissions.length()) {
        return;
    }

    FlatpakPermission &permission = m_permissions[index];

    switch (permission.section()) {
    case FlatpakPermissionsSectionType::Filesystems:
        editFilesystemsPermissions(permission, newValue.toString());
        break;
    case FlatpakPermissionsSectionType::SessionBus:
    case FlatpakPermissionsSectionType::SystemBus:
        editBusPermissions(permission, newValue.value<FlatpakPolicy>());
        break;
    case FlatpakPermissionsSectionType::Environment:
        editEnvPermission(permission, newValue.toString());
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

void FlatpakPermissionModel::addUserEnteredPermission(int /*FlatpakPermissionsSectionType::Type*/ rawSection, const QString &name, const QVariant &value)
{
    if (!QMetaEnum::fromType<FlatpakPermissionsSectionType::Type>().valueToKey(rawSection)) {
        return;
    }
    // SAFETY: QMetaEnum above ensures that coercion is valid.
    const auto section = static_cast<FlatpakPermissionsSectionType::Type>(rawSection);
    QString category;
    FlatpakPermission::Variant variant;

    switch (section) {
    case FlatpakPermissionsSectionType::Filesystems:
        category = QLatin1String(FLATPAK_METADATA_KEY_FILESYSTEMS);
        variant = value.toString();
        break;
    case FlatpakPermissionsSectionType::SessionBus:
        category = QLatin1String(FLATPAK_METADATA_GROUP_SESSION_BUS_POLICY);
        variant = value.value<FlatpakPolicy>();
        break;
    case FlatpakPermissionsSectionType::SystemBus:
        category = QLatin1String(FLATPAK_METADATA_GROUP_SYSTEM_BUS_POLICY);
        variant = value.value<FlatpakPolicy>();
        break;
    case FlatpakPermissionsSectionType::Environment:
        category = QLatin1String(FLATPAK_METADATA_GROUP_ENVIRONMENT);
        variant = value.toString();
        break;
    case FlatpakPermissionsSectionType::Basic:
    case FlatpakPermissionsSectionType::Advanced:
    case FlatpakPermissionsSectionType::SubsystemsShared:
    case FlatpakPermissionsSectionType::Sockets:
    case FlatpakPermissionsSectionType::Devices:
    case FlatpakPermissionsSectionType::Features:
        return;
    }

    FlatpakPermission permission(section, name, category, name, false, variant);
    permission.setOriginType(FlatpakPermission::OriginType::UserDefined);
    permission.setEffectiveEnabled(true);
    permission.setEffectiveValue(variant);

    int index = permIndex(category);
    m_permissions.insert(index, permission);

    switch (section) {
    case FlatpakPermissionsSectionType::Filesystems:
        addPermission(permission, true);
        break;
    case FlatpakPermissionsSectionType::SessionBus:
    case FlatpakPermissionsSectionType::SystemBus:
        addBusPermissions(permission);
        break;
    case FlatpakPermissionsSectionType::Environment:
        addEnvPermission(permission);
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

void FlatpakPermissionModel::addPermission(FlatpakPermission &permission, bool shouldBeOn)
{
    if (!m_overridesData.contains(QStringLiteral("[Context]"))) {
        m_overridesData.insert(m_overridesData.length(), QStringLiteral("[Context]\n"));
    }
    int catIndex = m_overridesData.indexOf(permission.category());

    /* if no permission from this category has been changed from default, we need to add the category */
    if (catIndex == -1) {
        catIndex = m_overridesData.indexOf(QLatin1Char('\n'), m_overridesData.indexOf(QStringLiteral("[Context]"))) + 1;
        if (catIndex == m_overridesData.length()) {
            m_overridesData.append(permission.category() + QLatin1String("=\n"));
        } else {
            m_overridesData.insert(catIndex, permission.category() + QLatin1String("=\n"));
        }
    }
    QString name = permission.name(); /* the name of the permission we are about to set/unset */
    if (permission.valueType() == FlatpakPermission::ValueType::Filesystems) {
        name.append(QLatin1Char(':') + permission.fsCurrentValue());
    }
    int permIndex = catIndex + permission.category().length() + 1;

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

void FlatpakPermissionModel::removePermission(FlatpakPermission &permission, bool isGranted)
{
    int permStartIndex = m_overridesData.indexOf(permission.name());
    int permEndIndex = permStartIndex + permission.name().length();

    if (permStartIndex == -1) {
        return;
    }

    /* if it is not granted, there exists a ! one index before the name that should also be deleted */
    if (!isGranted) {
        permStartIndex--;
    }

    if (permission.valueType() == FlatpakPermission::ValueType::Filesystems) {
        if (m_overridesData[permEndIndex] == QLatin1Char(':')) {
            permEndIndex += permission.fsCurrentValue().length() + 1;
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
    int catIndex = m_overridesData.indexOf(permission.category());
    if (m_overridesData[m_overridesData.indexOf(QLatin1Char('='), catIndex) + 1] == QLatin1Char('\n')) {
        m_overridesData.remove(catIndex, permission.category().length() + 2); // 2 because we need to remove '\n' as well
    }
}

void FlatpakPermissionModel::addBusPermissions(FlatpakPermission &permission)
{
    QString groupHeader = QLatin1Char('[') + permission.category() + QLatin1Char(']');
    if (!m_overridesData.contains(groupHeader)) {
        m_overridesData.insert(m_overridesData.length(), groupHeader + QLatin1Char('\n'));
    }
    const int permIndex = m_overridesData.indexOf(QLatin1Char('\n'), m_overridesData.indexOf(groupHeader)) + 1;

    const auto policyValue = std::get<FlatpakPolicy>(permission.effectiveValue());
    const auto policyString = mapDBusFlatpakPolicyEnumValueToConfigString(policyValue);

    m_overridesData.insert(permIndex, permission.name() + QLatin1Char('=') + policyString + QLatin1Char('\n'));
}

void FlatpakPermissionModel::removeBusPermission(FlatpakPermission &permission)
{
    int permBeginIndex = m_overridesData.indexOf(permission.name() + QLatin1Char('='));
    if (permBeginIndex == -1) {
        return;
    }

    const auto policyValue = std::get<FlatpakPolicy>(permission.effectiveValue());
    const auto policyString = mapDBusFlatpakPolicyEnumValueToConfigString(policyValue);

    int permLen = permission.name().length() + 1 + policyString.length() + 1;
    m_overridesData.remove(permBeginIndex, permLen);
}

void FlatpakPermissionModel::editFilesystemsPermissions(FlatpakPermission &permission, const QString &newValue)
{
    QString value;
    if (newValue == i18n("read-only")) {
        value = QStringLiteral(":ro");
    } else if (newValue == i18n("create")) {
        value = QStringLiteral(":create");
    } else {
        value = QStringLiteral(":rw");
    }

    int permIndex = m_overridesData.indexOf(permission.name());
    if (permIndex == -1) {
        addPermission(permission, true);
        permIndex = m_overridesData.indexOf(permission.name());
    }

    if (permission.isDefaultEnabled() == permission.isEffectiveEnabled() && std::get<QString>(permission.defaultValue()) == newValue) {
        removePermission(permission, true);
        permission.setEffectiveValue(newValue);
        return;
    }

    int valueBeginIndex = permIndex + permission.name().length();
    if (m_overridesData[valueBeginIndex] != QLatin1Char(':')) {
        m_overridesData.insert(permIndex + permission.name().length(), value);
    } else {
        m_overridesData.insert(valueBeginIndex, value);
        if (m_overridesData[valueBeginIndex + value.length() + 1] == QLatin1Char('r')) {
            m_overridesData.remove(valueBeginIndex + value.length(), 3);
        } else {
            m_overridesData.remove(valueBeginIndex + value.length(), 7);
        }
    }
    permission.setEffectiveValue(newValue);
}

void FlatpakPermissionModel::editBusPermissions(FlatpakPermission &permission, FlatpakPolicy newPolicyValue)
{
    if (permission.isDefaultEnabled() && newPolicyValue == std::get<FlatpakPolicy>(permission.overrideValue())) {
        permission.setEffectiveValue(newPolicyValue);
        removeBusPermission(permission);
        return;
    }

    int permIndex = m_overridesData.indexOf(permission.name());
    if (permIndex == -1) {
        addBusPermissions(permission);
        permIndex = m_overridesData.indexOf(permission.name());
    }
    int valueBeginIndex = permIndex + permission.name().length();
    const auto policyString = mapDBusFlatpakPolicyEnumValueToConfigString(newPolicyValue);
    m_overridesData.insert(valueBeginIndex, QLatin1Char('=') + policyString);
    valueBeginIndex = valueBeginIndex + 1 /* '=' character */ + policyString.length();
    auto valueEndOfLine = m_overridesData.indexOf(QLatin1Char('\n'), valueBeginIndex);
    m_overridesData.remove(valueBeginIndex, valueEndOfLine - valueBeginIndex);
    permission.setEffectiveValue(newPolicyValue);
}

void FlatpakPermissionModel::addEnvPermission(FlatpakPermission &permission)
{
    QString groupHeader = QLatin1Char('[') + permission.category() + QLatin1Char(']');
    if (!m_overridesData.contains(groupHeader)) {
        m_overridesData.insert(m_overridesData.length(), groupHeader + QLatin1Char('\n'));
    }
    int permIndex = m_overridesData.indexOf(QLatin1Char('\n'), m_overridesData.indexOf(groupHeader)) + 1;
    const auto value = permission.isEffectiveEnabled() ? std::get<QString>(permission.effectiveValue()) : QString();
    m_overridesData.insert(permIndex, permission.name() + QLatin1Char('=') + value + QLatin1Char('\n'));
}

void FlatpakPermissionModel::removeEnvPermission(FlatpakPermission &permission)
{
    int permBeginIndex = m_overridesData.indexOf(permission.name());
    if (permBeginIndex == -1) {
        return;
    }

    const auto value = permission.isEffectiveEnabled() ? std::get<QString>(permission.effectiveValue()) : QString();
    const int valueLen = permission.isEffectiveEnabled() ? value.length() + 1 : 0;

    const int permLen = permission.name().length() + 1 + valueLen;
    m_overridesData.remove(permBeginIndex, permLen);
}

void FlatpakPermissionModel::editEnvPermission(FlatpakPermission &permission, const QString &value)
{
    if (permission.isDefaultEnabled() && value == std::get<QString>(permission.defaultValue())) {
        removeEnvPermission(permission);
        return;
    }

    int permIndex = m_overridesData.indexOf(permission.name());
    if (permIndex == -1) {
        addEnvPermission(permission);
        permIndex = m_overridesData.indexOf(permission.name());
    }
    int valueBeginIndex = permIndex + permission.name().length();
    m_overridesData.insert(valueBeginIndex, QLatin1Char('=') + value);

    int oldValBeginIndex = valueBeginIndex + value.length() + 1;
    int oldValEndIndex = m_overridesData.indexOf(QLatin1Char('\n'), oldValBeginIndex);
    m_overridesData.remove(oldValBeginIndex, oldValEndIndex - oldValBeginIndex);

    if (!value.isEmpty()) {
        permission.setEffectiveValue(value);
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
