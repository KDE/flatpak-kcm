/**
 * SPDX-FileCopyrightText: 2022 Suhaas Joshi <joshiesuhaas0@gmail.com>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kcm.h"

#include <KPluginFactory>
#include <KLocalizedString>
#include <QFile>

K_PLUGIN_CLASS_WITH_JSON(KCMFlatpak, "kcm_flatpak.json")

KCMFlatpak::KCMFlatpak(QObject *parent, const KPluginMetaData &data, const QVariantList &args)
    : KQuickAddons::ManagedConfigModule(parent, data, args),
      m_refsModel(new FlatpakReferencesModel(this))
{
    qmlRegisterUncreatableType<KCMFlatpak>("org.kde.plasma.kcm.flatpakpermissions", 1, 0, "KCMFlatpak", QString());
    qmlRegisterType<FlatpakPermissionModel>("org.kde.plasma.kcm.flatpakpermissions", 1, 0, "FlatpakPermissionModel");

    connect(m_refsModel, &FlatpakReferencesModel::needsLoad, this, &KCMFlatpak::load);
    connect(m_refsModel, &FlatpakReferencesModel::needsSaveChanged, this, &KCMFlatpak::refreshSaveNeeded);
}

void KCMFlatpak::refreshSaveNeeded()
{
    setNeedsSave(isSaveNeeded());
}

void KCMFlatpak::load()
{
    m_refsModel->load(m_index);
    setNeedsSave(false);
}

void KCMFlatpak::save()
{
    m_refsModel->save(m_index);
}

void KCMFlatpak::defaults()
{
    m_refsModel->defaults(m_index);
}

bool KCMFlatpak::isSaveNeeded() const
{
    return m_refsModel->isSaveNeeded(m_index);
}

bool KCMFlatpak::isDefaults() const
{
    return m_refsModel->isDefaults(m_index);
}

void KCMFlatpak::setIndex(int index)
{
    m_index = index;
}

#include "kcm.moc"
