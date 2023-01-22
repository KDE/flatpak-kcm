/**
 * SPDX-FileCopyrightText: 2022 Suhaas Joshi <joshiesuhaas0@gmail.com>
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

FlatpakReference::~FlatpakReference() = default;

FlatpakReference::FlatpakReference(FlatpakReferencesModel *parent, QString name, QString id, const QString &path, QString version, QString icon, QByteArray metadata, FlatpakReferencesModel *refsModel)
    : QObject(parent),
      m_name(name),
      m_id(id),
      m_version(version),
      m_icon(icon),
      m_path(path),
      m_metadata(metadata),
      m_refsModel(refsModel)
{
    m_path.append(m_id);
}

QString FlatpakReference::name() const
{
    return m_name;
}

QString FlatpakReference::displayName() const
{
    /* sometimes, the application does not seem to have a display name, so return its id */
    return m_name.isEmpty() ? m_id : m_name;
}

QString FlatpakReference::version() const
{
    return m_version;
}

QString FlatpakReference::icon() const
{
    return m_icon;
}

QString FlatpakReference::path() const
{
    return m_path;
}

QByteArray FlatpakReference::metadata() const
{
    return m_metadata;
}

FlatpakPermissionModel *FlatpakReference::permsModel()
{
    return m_permsModel;
}

void FlatpakReference::setPermsModel(FlatpakPermissionModel *permsModel)
{
    m_permsModel = permsModel;
    connect(m_permsModel, &FlatpakPermissionModel::referenceChanged, this, &FlatpakReference::needsLoad);
    connect(this, &FlatpakReference::needsLoad, m_refsModel, &FlatpakReferencesModel::needsLoad);
    connect(m_permsModel, &FlatpakPermissionModel::dataChanged, this, &FlatpakReference::needsSaveChanged);
    connect(this, &FlatpakReference::needsSaveChanged, m_refsModel, &FlatpakReferencesModel::needsSaveChanged);
}

void FlatpakReference::load()
{
    m_permsModel->load();
}

void FlatpakReference::save()
{
    m_permsModel->save();
}

void FlatpakReference::defaults()
{
    m_permsModel->defaults();
}

bool FlatpakReference::isSaveNeeded() const
{
    return m_permsModel->isSaveNeeded();
}

bool FlatpakReference::isDefaults() const
{
    return m_permsModel->isDefaults();
}

FlatpakReferencesModel::FlatpakReferencesModel(QObject *parent) : QAbstractListModel(parent)
{
    g_autoptr(FlatpakInstallation) installation = flatpak_installation_new_system(NULL, NULL);
    g_autoptr(GPtrArray) installedApps = flatpak_installation_list_installed_refs_by_kind(installation, FLATPAK_REF_KIND_APP, NULL, NULL);
    g_autoptr(FlatpakInstallation) userInstallation = flatpak_installation_new_user(NULL, NULL);
    g_autoptr(GPtrArray) installedUserApps = flatpak_installation_list_installed_refs_by_kind(userInstallation, FLATPAK_REF_KIND_APP, NULL, NULL);
    g_ptr_array_extend_and_steal(installedApps, installedUserApps);
    QString path = FlatpakHelper::permDataFilePath();

    for(uint i = 0; i < installedApps->len; ++i) {
        QString name = QString::fromUtf8(flatpak_installed_ref_get_appdata_name(FLATPAK_INSTALLED_REF(g_ptr_array_index(installedApps, i))));
        QString version = QString::fromUtf8(flatpak_installed_ref_get_appdata_version(FLATPAK_INSTALLED_REF(g_ptr_array_index(installedApps, i))));
        QString id = QString::fromUtf8(flatpak_ref_get_name(FLATPAK_REF(g_ptr_array_index(installedApps, i))));
        if (id.endsWith(QStringLiteral(".BaseApp"))) {
            continue;
        }
        QString icon = FlatpakHelper::iconPath(name, id);

        g_autoptr(GBytes) data = flatpak_installed_ref_load_metadata(FLATPAK_INSTALLED_REF(g_ptr_array_index(installedApps, i)), NULL, NULL);
        gsize len = 0;
        auto buff = g_bytes_get_data(data, &len);
        const QByteArray metadata((const char *)buff, len);

        m_references.push_back(new FlatpakReference(this, name, id, path, version, icon, metadata, this));
    }
    std::sort(m_references.begin(), m_references.end(), [] (FlatpakReference *r1, FlatpakReference *r2) {return r1->name() < r2->name(); });
}

int FlatpakReferencesModel::rowCount(const QModelIndex &parent) const
{
    if(parent.isValid()) {
        return 0;
    }
    return m_references.count();
}

QVariant FlatpakReferencesModel::data(const QModelIndex &index, int role) const
{
    if(!index.isValid()) {
        return QVariant();
    }

    switch(role) {
    case Roles::Name:
        return m_references.at(index.row())->displayName();
    case Roles::Version:
        return m_references.at(index.row())->version();
    case Roles::Icon:
        return m_references.at(index.row())->icon();
    case Roles::Ref:
        return QVariant::fromValue<FlatpakReference*>(m_references.at(index.row()));
    }
    return QVariant();
}

QHash<int, QByteArray> FlatpakReferencesModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[Roles::Name] = "name";
    roles[Roles::Version] = "version";
    roles[Roles::Icon] = "icon";
    roles[Roles::Ref] = "reference";
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
