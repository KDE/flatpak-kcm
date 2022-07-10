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
    //qmlRegisterType<FlatpakReference>("org.kde.plasma.kcm.flatpakpermissions", 1, 0, "FlatpakReference");
}

#include "kcm.moc"
