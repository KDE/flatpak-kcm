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

FlatpakReference::FlatpakReference(FlatpakReferencesModel *parent,
                                   const QString &flatpakName,
                                   const QString &arch,
                                   const QString &branch,
                                   const QString &version,
                                   const QString &displayName,
                                   const QString &permissionsDirectory,
                                   const QUrl &iconSource,
                                   const QByteArray &metadata)
    : QObject(parent)
    , m_flatpakName(flatpakName)
    , m_arch(arch)
    , m_branch(branch)
    , m_version(version)
    , m_displayName(displayName)
    , m_iconSource(iconSource)
    , m_permissionsFilename(permissionsDirectory)
    , m_metadata(metadata)
    , m_permissionsModel(nullptr)
{
    m_permissionsFilename.append(m_flatpakName);

    connect(this, &FlatpakReference::needsLoad, parent, &FlatpakReferencesModel::needsLoad);
    connect(this, &FlatpakReference::settingsChanged, parent, &FlatpakReferencesModel::settingsChanged);
}

FlatpakReferencesModel *FlatpakReference::parent() const
{
    // SAFETY: There's only one constructor, and it always initializes parent with a model object
    return qobject_cast<FlatpakReferencesModel *>(QObject::parent());
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

QString FlatpakReference::permissionsFilename() const
{
    return m_permissionsFilename;
}

QByteArray FlatpakReference::metadata() const
{
    return m_metadata;
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
        if (m_permissionsModel) {
            disconnect(m_permissionsModel, &FlatpakPermissionModel::referenceChanged, this, &FlatpakReference::needsLoad);
            disconnect(m_permissionsModel, &FlatpakPermissionModel::dataChanged, this, &FlatpakReference::settingsChanged);
            disconnect(m_permissionsModel, &FlatpakPermissionModel::rowsInserted, this, &FlatpakReference::settingsChanged);
            disconnect(m_permissionsModel, &FlatpakPermissionModel::rowsRemoved, this, &FlatpakReference::settingsChanged);
        }
        m_permissionsModel = model;
        if (m_permissionsModel) {
            connect(m_permissionsModel, &FlatpakPermissionModel::referenceChanged, this, &FlatpakReference::needsLoad);
            connect(m_permissionsModel, &FlatpakPermissionModel::dataChanged, this, &FlatpakReference::settingsChanged);
            connect(m_permissionsModel, &FlatpakPermissionModel::rowsInserted, this, &FlatpakReference::settingsChanged);
            connect(m_permissionsModel, &FlatpakPermissionModel::rowsRemoved, this, &FlatpakReference::settingsChanged);
        }
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

static QByteArray qByteArrayFromGBytes(GBytes *data)
{
    gsize len = 0;
    auto buff = g_bytes_get_data(data, &len);
    return QByteArray((const char *)buff, len);
}

FlatpakReferencesModel::FlatpakReferencesModel(QObject *parent)
    : QAbstractListModel(parent)
{
    g_autoptr(FlatpakInstallation) systemInstallation = flatpak_installation_new_system(nullptr, nullptr);
    g_autoptr(GPtrArray) installedApps = flatpak_installation_list_installed_refs_by_kind(systemInstallation, FLATPAK_REF_KIND_APP, nullptr, nullptr);
    g_autoptr(FlatpakInstallation) userInstallation = flatpak_installation_new_user(nullptr, nullptr);
    // it's the only pointer, so extend_and_steal will destroy it.
    GPtrArray *installedUserApps = flatpak_installation_list_installed_refs_by_kind(userInstallation, FLATPAK_REF_KIND_APP, nullptr, nullptr);
    g_ptr_array_extend_and_steal(installedApps, installedUserApps);
    installedUserApps = nullptr;
    QString permissionsDirectory = FlatpakHelper::permissionsDataDirectory();

    for (uint i = 0; i < installedApps->len; ++i) {
        auto *ref = FLATPAK_REF(g_ptr_array_index(installedApps, i));
        auto *iRef = FLATPAK_INSTALLED_REF(g_ptr_array_index(installedApps, i));

        const QString flatpakName = QString::fromUtf8(flatpak_ref_get_name(ref));
        if (flatpakName.endsWith(QStringLiteral(".BaseApp"))) {
            continue;
        }

        const QString arch = QString::fromUtf8(flatpak_ref_get_arch(ref));
        const QString branch = QString::fromUtf8(flatpak_ref_get_branch(ref));
        const QString version = QString::fromUtf8(flatpak_installed_ref_get_appdata_version(iRef));
        const QString displayName = QString::fromUtf8(flatpak_installed_ref_get_appdata_name(iRef));
        const QString appBasePath = QString::fromUtf8(flatpak_installed_ref_get_deploy_dir(iRef));
        const QUrl iconSource = FlatpakHelper::iconSourceUrl(displayName, flatpakName, appBasePath);

        g_autoptr(GBytes) data = flatpak_installed_ref_load_metadata(iRef, nullptr, nullptr);
        const QByteArray metadata = qByteArrayFromGBytes(data);

        m_references.push_back(new FlatpakReference(this, flatpakName, arch, branch, version, displayName, permissionsDirectory, iconSource, metadata));
    }
    std::sort(m_references.begin(), m_references.end(), [](const FlatpakReference *r1, const FlatpakReference *r2) {
        return r1->displayName() < r2->displayName();
    });
}

int FlatpakReferencesModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return m_references.count();
}

QVariant FlatpakReferencesModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }

    switch (role) {
    case Roles::Name:
        return m_references.at(index.row())->displayName();
    case Roles::Version:
        return m_references.at(index.row())->version();
    case Roles::Icon:
        return m_references.at(index.row())->iconSource();
    case Roles::Ref:
        return QVariant::fromValue<FlatpakReference *>(m_references.at(index.row()));
    }
    return QVariant();
}

QHash<int, QByteArray> FlatpakReferencesModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[Roles::Name] = "name";
    roles[Roles::Version] = "version";
    roles[Roles::Icon] = "icon";
    roles[Roles::Ref] = "ref";
    return roles;
}

void FlatpakReferencesModel::load(int index)
{
    if (index >= 0 && index < m_references.length()) {
        m_references.at(index)->load();
    }
}

void FlatpakReferencesModel::save(int index)
{
    if (index >= 0 && index < m_references.length()) {
        m_references.at(index)->save();
    }
}

void FlatpakReferencesModel::defaults(int index)
{
    if (index >= 0 && index < m_references.length()) {
        m_references.at(index)->defaults();
    }
}

bool FlatpakReferencesModel::isSaveNeeded(int index) const
{
    if (index >= 0 && index < m_references.length()) {
        return m_references.at(index)->isSaveNeeded();
    }
    return false;
}

bool FlatpakReferencesModel::isDefaults(int index) const
{
    if (index >= 0 && index < m_references.length()) {
        return m_references.at(index)->isDefaults();
    }
    return true;
}

const QVector<FlatpakReference *> &FlatpakReferencesModel::references() const
{
    return m_references;
}
