/**
 * SPDX-FileCopyrightText: 2022 Suhaas Joshi <joshiesuhaas0@gmail.com>
 * SPDX-FileCopyrightText: 2023 ivan tkachenko <me@ratijas.tk>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "flatpakreference.h"
#include "flatpakhelper.h"

#ifdef FLATPAK_EXTERNC_REQUIRED
extern "C" {
#endif
#include <flatpak/flatpak.h>
#ifdef FLATPAK_EXTERNC_REQUIRED
}
#endif
#include <glib.h>

#include <QDebug>

FlatpakReference::FlatpakReference(const QString &flatpakName,
                                   const QString &arch,
                                   const QString &branch,
                                   const QString &version,
                                   const QString &displayName,
                                   const QUrl &iconSource,
                                   const QStringList &metadataAndOverridesFiles)
    : m_flatpakName(flatpakName)
    , m_arch(arch)
    , m_branch(branch)
    , m_version(version)
    , m_displayName(displayName)
    , m_iconSource(iconSource)
    , m_metadataAndOverridesFiles(metadataAndOverridesFiles)
    , m_permissionsModel(nullptr)
{
}

QString FlatpakReference::arch() const
{
    return m_arch;
}

QString FlatpakReference::branch() const
{
    return m_branch;
}

QString FlatpakReference::version() const
{
    return m_version;
}

QUrl FlatpakReference::iconSource() const
{
    return m_iconSource;
}

const QStringList &FlatpakReference::metadataAndOverridesFiles() const
{
    return m_metadataAndOverridesFiles;
}

QStringList FlatpakReference::defaultsFiles() const
{
    QStringList defaults = m_metadataAndOverridesFiles;
    defaults.removeLast();
    return defaults;
}

const QString &FlatpakReference::userLevelPerAppOverrideFile() const
{
    return m_metadataAndOverridesFiles.last();
}

QString FlatpakReference::displayName() const
{
    /* sometimes, the application does not seem to have a display name, so return its id */
    return m_displayName.isEmpty() ? m_flatpakName : m_displayName;
}

QString FlatpakReference::flatpakName() const
{
    // Reduced implementation of libdiscover, as this KCM lists only installed apps
    return m_flatpakName;
}

QString FlatpakReference::ref() const
{
    // KCM lists only apps
    return QStringLiteral("app/%1/%2/%3").arg(flatpakName(), arch(), branch());
}

FlatpakPermissionModel *FlatpakReference::permissionsModel()
{
    return m_permissionsModel;
}

void FlatpakReference::setPermissionsModel(FlatpakPermissionModel *model)
{
    if (model != m_permissionsModel) {
        m_permissionsModel = model;
    }
}

void FlatpakReference::load()
{
    if (m_permissionsModel) {
        m_permissionsModel->load();
    }
}

void FlatpakReference::save()
{
    if (m_permissionsModel) {
        m_permissionsModel->save();
    }
}

void FlatpakReference::defaults()
{
    if (m_permissionsModel) {
        m_permissionsModel->defaults();
    }
}

bool FlatpakReference::isSaveNeeded() const
{
    if (m_permissionsModel) {
        return m_permissionsModel->isSaveNeeded();
    }
    return false;
}

bool FlatpakReference::isDefaults() const
{
    if (m_permissionsModel) {
        return m_permissionsModel->isDefaults();
    }
    return true;
}

static GPtrArray *getSystemInstalledFlatpakAppRefs()
{
    auto ret = g_ptr_array_new();

    g_autoptr(GPtrArray) installations = flatpak_get_system_installations(nullptr, nullptr);
    g_ptr_array_foreach(
        installations,
        [](gpointer data, gpointer user_data) {
            auto installation = FLATPAK_INSTALLATION(data);
            auto ret = static_cast<GPtrArray *>(user_data);
            auto refs = flatpak_installation_list_installed_refs_by_kind(installation, FLATPAK_REF_KIND_APP, nullptr, nullptr);
            g_ptr_array_extend_and_steal(ret, refs);
        },
        ret);

    return ret;
}

static GPtrArray *getUserInstalledFlatpakAppRefs()
{
    g_autoptr(FlatpakInstallation) installation = flatpak_installation_new_user(nullptr, nullptr);
    GPtrArray *refs = flatpak_installation_list_installed_refs_by_kind(installation, FLATPAK_REF_KIND_APP, nullptr, nullptr);
    return refs;
}

std::vector<std::unique_ptr<FlatpakReference>> FlatpakReference::allFlatpakReferences()
{
    g_autoptr(GPtrArray) systemInstalledRefs = getSystemInstalledFlatpakAppRefs();
    g_autoptr(GPtrArray) userInstalledRefs = getUserInstalledFlatpakAppRefs();

    const auto systemOverridesDirectory = FlatpakHelper::systemOverridesDirectory();
    const auto userOverridesDirectory = FlatpakHelper::userOverridesDirectory();

    std::vector<std::unique_ptr<FlatpakReference>> references;

    for (const auto &refs : {systemInstalledRefs, userInstalledRefs}) {
        for (uint i = 0; i < refs->len; ++i) {
            auto *ref = FLATPAK_REF(g_ptr_array_index(refs, i));
            auto *iRef = FLATPAK_INSTALLED_REF(g_ptr_array_index(refs, i));

            const auto flatpakName = QString::fromUtf8(flatpak_ref_get_name(ref));
            if (flatpakName.endsWith(QStringLiteral(".BaseApp"))) {
                continue;
            }

            const auto arch = QString::fromUtf8(flatpak_ref_get_arch(ref));
            const auto branch = QString::fromUtf8(flatpak_ref_get_branch(ref));
            const auto version = QString::fromUtf8(flatpak_installed_ref_get_appdata_version(iRef));
            const auto displayName = QString::fromUtf8(flatpak_installed_ref_get_appdata_name(iRef));
            const auto appBaseDirectory = QString::fromUtf8(flatpak_installed_ref_get_deploy_dir(iRef));
            const auto iconSource = FlatpakHelper::iconSourceUrl(displayName, flatpakName, appBaseDirectory);

            const auto metadataPath = (refs == systemInstalledRefs) ? FlatpakHelper::metadataPathForSystemInstallation(flatpakName)
                                                                    : FlatpakHelper::metadataPathForUserInstallation(flatpakName);

            const auto systemGlobalOverrides = QStringLiteral("%1/global").arg(systemOverridesDirectory);
            const auto systemAppOverrides = QStringLiteral("%1/%2").arg(systemOverridesDirectory, flatpakName);

            const auto userGlobalOverrides = QStringLiteral("%1/global").arg(userOverridesDirectory);
            const auto userAppOverrides = QStringLiteral("%1/%2").arg(userOverridesDirectory, flatpakName);

            const auto metadataAndOverridesFiles = QStringList({
                metadataPath,
                systemGlobalOverrides,
                systemAppOverrides,
                userGlobalOverrides,
                userAppOverrides,
            });

            references.push_back(std::make_unique<FlatpakReference>(flatpakName, arch, branch, version, displayName, iconSource, metadataAndOverridesFiles));
        }
    }
    return references;
}

#include "moc_flatpakreference.cpp"
