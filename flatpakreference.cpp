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
#include <QDebug>

FlatpakReference::~FlatpakReference() = default;

FlatpakReference::FlatpakReference(
    FlatpakReferencesModel *parent,
    const QString &name,
    const QString &id,
    const QString &path,
    const QString &version,
    const QString &icon,
    const QByteArray &metadata
) : QObject(parent),
    m_name(name),
    m_id(id),
    m_version(version),
    m_icon(icon),
    m_path(path),
    m_metadata(metadata),
    m_permsModel(nullptr)
{
    m_path.append(m_id);
}

FlatpakReferencesModel *FlatpakReference::parent() const
{
    // SAFETY: There's only one constructor, and it always initializes parent with a model object
    return static_cast<FlatpakReferencesModel *>(QObject::parent());
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
    if (permsModel != m_permsModel) {
        if (m_permsModel) {
            disconnect(m_permsModel, &FlatpakPermissionModel::referenceChanged, this, &FlatpakReference::needsLoad);
            disconnect(this, &FlatpakReference::needsLoad, parent(), &FlatpakReferencesModel::needsLoad);
            disconnect(m_permsModel, &FlatpakPermissionModel::dataChanged, this, &FlatpakReference::needsSaveChanged);
            disconnect(this, &FlatpakReference::needsSaveChanged, parent(), &FlatpakReferencesModel::needsSaveChanged);
        }
        m_permsModel = permsModel;
        if (m_permsModel) {
            connect(m_permsModel, &FlatpakPermissionModel::referenceChanged, this, &FlatpakReference::needsLoad);
            connect(this, &FlatpakReference::needsLoad, parent(), &FlatpakReferencesModel::needsLoad);
            connect(m_permsModel, &FlatpakPermissionModel::dataChanged, this, &FlatpakReference::needsSaveChanged);
            connect(this, &FlatpakReference::needsSaveChanged, parent(), &FlatpakReferencesModel::needsSaveChanged);
        }
    }
}

void FlatpakReference::load()
{
    if (m_permsModel) {
        m_permsModel->load();
    }
}

void FlatpakReference::save()
{
    if (m_permsModel) {
        m_permsModel->save();
    }
}

void FlatpakReference::defaults()
{
    if (m_permsModel) {
        m_permsModel->defaults();
    }
}

bool FlatpakReference::isSaveNeeded() const
{
    if (m_permsModel) {
        return m_permsModel->isSaveNeeded();
    }
    return false;
}

bool FlatpakReference::isDefaults() const
{
    if (m_permsModel) {
        return m_permsModel->isDefaults();
    }
    return true;
}

static QByteArray qByteArrayFromGBytes(GBytes *data)
{
    gsize len = 0;
    auto buff = g_bytes_get_data(data, &len);
    return QByteArray((const char *)buff, len);
}

FlatpakReferencesModel::FlatpakReferencesModel(QObject *parent) : QAbstractListModel(parent)
{
    g_autoptr(FlatpakInstallation) installation = flatpak_installation_new_system(nullptr, nullptr);
    g_autoptr(GPtrArray) installedApps = flatpak_installation_list_installed_refs_by_kind(installation, FLATPAK_REF_KIND_APP, nullptr, nullptr);
    g_autoptr(FlatpakInstallation) userInstallation = flatpak_installation_new_user(nullptr, nullptr);
    g_autoptr(GPtrArray) installedUserApps = flatpak_installation_list_installed_refs_by_kind(userInstallation, FLATPAK_REF_KIND_APP, nullptr, nullptr);
    g_ptr_array_extend_and_steal(installedApps, installedUserApps);
    QString path = FlatpakHelper::permDataFilePath();

    for (uint i = 0; i < installedApps->len; ++i) {
        auto *ref = FLATPAK_REF(g_ptr_array_index(installedApps, i));
        auto *iRef = FLATPAK_INSTALLED_REF(g_ptr_array_index(installedApps, i));

        QString name = QString::fromUtf8(flatpak_installed_ref_get_appdata_name(iRef));
        QString version = QString::fromUtf8(flatpak_installed_ref_get_appdata_version(iRef));
        QString id = QString::fromUtf8(flatpak_ref_get_name(ref));
        if (id.endsWith(QStringLiteral(".BaseApp"))) {
            continue;
        }
        QString appBasePath = QString::fromUtf8(flatpak_installed_ref_get_deploy_dir(iRef));
        QString icon = FlatpakHelper::iconPath(name, id, appBasePath);

        g_autoptr(GBytes) data = flatpak_installed_ref_load_metadata(iRef, nullptr, nullptr);
        const QByteArray metadata = qByteArrayFromGBytes(data);

        m_references.push_back(new FlatpakReference(
            this,
            name,
            id,
            path,
            version,
            icon,
            metadata
        ));
    }
    std::sort(m_references.begin(), m_references.end(), [] (FlatpakReference *r1, FlatpakReference *r2) {return r1->name() < r2->name(); });
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

const QVector<FlatpakReference *> &FlatpakReferencesModel::references() const
{
    return m_references;
}
