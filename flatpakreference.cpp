/**
 * SPDX-FileCopyrightText: 2022 Suhaas Joshi <joshiesuhaas0@gmail.com>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "flatpakreference.h"

#ifdef FLATPAK_EXTERNC_REQUIRED
extern "C" {
#endif
#include <flatpak/flatpak.h>
#ifdef FLATPAK_EXTERNC_REQUIRED
}
#endif
#include <glib.h>

#include <QDebug>

FlatpakReference::FlatpakReference(QString name, QString version, QString icon)
    : m_name(name),
      m_version(version),
      m_icon(icon)
{
}

QString FlatpakReference::name() const
{
    return m_name;
}

QString FlatpakReference::version() const
{
    return m_version;
}

QString FlatpakReference::icon() const
{
    return m_icon;
}

FlatpakReferencesModel::FlatpakReferencesModel()
{
    g_autoptr(FlatpakInstallation) installation = flatpak_installation_new_system(NULL, NULL);
    g_autoptr(GPtrArray) installedApps = flatpak_installation_list_installed_refs_by_kind(installation, FLATPAK_REF_KIND_APP, NULL, NULL);

    for(uint i = 0; i < installedApps->len; ++i) {
        g_autoptr(FlatpakInstalledRef) installedRef = FLATPAK_INSTALLED_REF(g_ptr_array_index(installedApps, i));
        QString name = QString::fromUtf8(flatpak_installed_ref_get_appdata_name(installedRef));
        QString version = QString::fromUtf8(flatpak_installed_ref_get_appdata_version(installedRef));

        g_autoptr(FlatpakRef) ref = FLATPAK_REF(g_ptr_array_index(installedApps, i));
        QString id = QString::fromUtf8(flatpak_ref_get_name(ref));

        QString icon = id + QLatin1String(".png");

        m_references.push_back(FlatpakReference(name, version, icon));
    }
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
        return m_references.at(index.row()).name();
    case Roles::Version:
        return m_references.at(index.row()).version();
    case Roles::Icon:
        return m_references.at(index.row()).icon();
    }
    return QVariant();
}

QHash<int, QByteArray> FlatpakReferencesModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[Roles::Name] = "name";
    roles[Roles::Version] = "version";
    roles[Roles::Icon] = "icon";
    return roles;
}
