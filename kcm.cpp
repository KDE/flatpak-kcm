/**
 * SPDX-FileCopyrightText: 2022 Suhaas Joshi <joshiesuhaas0@gmail.com>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kcm.h"

#include <KLocalizedString>
#include <KPluginFactory>
#include <QFile>

K_PLUGIN_CLASS_WITH_JSON(KCMFlatpak, "kcm_flatpak.json")

KCMFlatpak::KCMFlatpak(QObject *parent, const KPluginMetaData &data, const QVariantList &args)
    : KQuickAddons::ManagedConfigModule(parent, data, args)
    , m_refsModel(new FlatpakReferencesModel(this))
{
    constexpr const char *uri = "org.kde.plasma.kcm.flatpakpermissions";

    qmlRegisterUncreatableType<KCMFlatpak>(uri, 1, 0, "KCMFlatpak", QString());
    qmlRegisterType<FlatpakPermissionModel>(uri, 1, 0, "FlatpakPermissionModel");
    qmlRegisterUncreatableType<FlatpakReferencesModel>(uri, 1, 0, "FlatpakReferencesModel", QStringLiteral("For enum access only"));
    qmlRegisterUncreatableType<FlatpakPermissionsSectionType>(uri, 1, 0, "FlatpakPermissionsSectionType", QStringLiteral("For enum access only"));

    connect(m_refsModel, &FlatpakReferencesModel::needsLoad, this, &KCMFlatpak::load);
    connect(m_refsModel, &FlatpakReferencesModel::settingsChanged, this, &KCMFlatpak::settingsChanged);
    settingsChanged(); // Initialize Reset & Defaults buttons
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
    settingsChanged(); // Because Apply, Reset & Defaults buttons depend on m_index.
}

int KCMFlatpak::currentIndex() const
{
    return m_index;
}

#include "kcm.moc"
