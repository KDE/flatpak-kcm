/**
 * SPDX-FileCopyrightText: 2022 Suhaas Joshi <joshiesuhaas0@gmail.com>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kcm.h"

#include <KLocalizedString>
#include <KPluginFactory>
#include <QFile>
#include <algorithm>

K_PLUGIN_CLASS_WITH_JSON(KCMFlatpak, "kcm_flatpak.json")

KCMFlatpak::KCMFlatpak(QObject *parent, const KPluginMetaData &data, const QVariantList &args)
    : KQuickAddons::ManagedConfigModule(parent, data, args)
    , m_refsModel(new FlatpakReferencesModel(this))
{
    QString requrestedRef;
    if (args.count() > 0) {
        const QVariant &arg0 = args.at(0);
        if (arg0.canConvert<QString>()) {
            const QString arg0str = arg0.toString();
            requrestedRef = arg0str;
        }
    }

    qmlRegisterUncreatableType<KCMFlatpak>("org.kde.plasma.kcm.flatpakpermissions", 1, 0, "KCMFlatpak", QString());
    qmlRegisterType<FlatpakPermissionModel>("org.kde.plasma.kcm.flatpakpermissions", 1, 0, "FlatpakPermissionModel");
    qmlRegisterUncreatableType<FlatpakReferencesModel>("org.kde.plasma.kcm.flatpakpermissions",
                                                       1,
                                                       0,
                                                       "FlatpakReferencesModel",
                                                       QStringLiteral("For enum access only"));

    connect(m_refsModel, &FlatpakReferencesModel::needsLoad, this, &KCMFlatpak::load);
    connect(m_refsModel, &FlatpakReferencesModel::needsSaveChanged, this, &KCMFlatpak::refreshSaveNeeded);

    if (!requrestedRef.isEmpty()) {
        const auto &refs = m_refsModel->references();
        const auto it = std::find_if(refs.constBegin(), refs.constEnd(), [&](FlatpakReference *ref) {
            return ref->ref() == requrestedRef;
        });
        if (it != refs.constEnd()) {
            const auto index = std::distance(refs.constBegin(), it);
            m_index = index;
        }
    }
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

int KCMFlatpak::currentIndex() const
{
    return m_index;
}

#include "kcm.moc"
