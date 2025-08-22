/**
 * SPDX-FileCopyrightText: 2022 Suhaas Joshi <joshiesuhaas0@gmail.com>
 * SPDX-FileCopyrightText: 2023 ivan tkachenko <me@ratijas.tk>
 * SPDX-FileCopyrightText: 2025 David Redondo <kde@david-redondo.de>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kcm.h"

#include "permissionitem.h"
#include "permissionstore.h"
#include "restoredatamodel.h"

#include <KLocalizedString>
#include <KPluginFactory>

#include <QQmlEngine>

#include <algorithm>

K_PLUGIN_CLASS_WITH_JSON(AppPermissionsKCM, "kcm_app-permissions.json")

using namespace Qt::StringLiterals;

AppPermissionsKCM::AppPermissionsKCM(QObject *parent, const KPluginMetaData &data, const QVariantList &args)
    : KQuickManagedConfigModule(parent, data)
    , m_references(FlatpakReference::allFlatpakReferences())
    , m_permissionStore(PermissionStore::instance())

{
    constexpr const char *uri = "org.kde.plasma.kcm.flatpakpermissions";

    qmlRegisterUncreatableType<FlatpakReference>(uri, 1, 0, "FlatpakReference", QStringLiteral("Should be obtained from the KCM"));
    qmlRegisterType<FlatpakPermissionModel>(uri, 1, 0, "FlatpakPermissionModel");
    qmlRegisterUncreatableType<FlatpakPermissionsSectionType>(uri, 1, 0, "FlatpakPermissionsSectionType", QStringLiteral("For enum access only"));

    qmlRegisterType<PermissionItem>(uri, 1, 0, "PermissionItem");
    qmlRegisterType<RemoteDesktopSessionsModel>(uri, 1, 0, "RemoteDesktopSessionsModel");
    qmlRegisterType<ScreencastSessionsModel>(uri, 1, 0, "ScreencastSessionsModel");
    m_permissionStore->loadTable("screenshot"_L1);
    m_permissionStore->loadTable("location"_L1);
    m_permissionStore->loadTable("notifications"_L1);
    m_permissionStore->loadTable("gamemode"_L1);
    m_permissionStore->loadTable("realtime"_L1);
    m_permissionStore->loadTable("devices"_L1); // For camera
    m_permissionStore->loadTable("remote-desktop"_L1);
    m_permissionStore->loadTable("screencast"_L1);
    m_permissionStore->loadTable("kde-authorized"_L1);

    auto reference = indexFromArgs(args);
    if (reference) {
        connect(
            this,
            &KQuickConfigModule::mainUiReady,
            this,
            [this, reference] {
                push("FlatpakPermissions.qml"_L1, {{u"ref"_s, QVariant::fromValue(reference)}});
            },
            Qt::SingleShotConnection);
    }
    connect(this, &KQuickConfigModule::activationRequested, this, [this](const QVariantList &args) {
        auto reference = indexFromArgs(args);
        if (reference) {
            push("FlatpakPermissions.qml"_L1, {{u"ref"_s, QVariant::fromValue(reference)}});
        }
    });
}

FlatpakReference *AppPermissionsKCM::indexFromArgs(const QVariantList &args) const
{
    if (args.isEmpty()) {
        return nullptr;
    }

    QString requestedReference;

    const QVariant &arg0 = args.at(0);
    if (arg0.canConvert<QString>()) {
        requestedReference = arg0.toString();
    } else {
        return nullptr;
    }

    const auto it = std::ranges::find_if(m_references.cbegin(), m_references.cend(), [&](const auto &ref) {
        return ref->ref() == requestedReference;
    });
    if (it != m_references.cend()) {
        return it->get();
    } else {
        return nullptr;
    }
}

void AppPermissionsKCM::load()
{
    std::ranges::for_each(m_references, &FlatpakReference::load);
}

void AppPermissionsKCM::save()
{
    std::ranges::for_each(m_references, &FlatpakReference::save);
}

void AppPermissionsKCM::defaults()
{
    std::ranges::for_each(m_references, &FlatpakReference::defaults);
}

const AppsModel *AppPermissionsKCM::appsModel() const
{
    return &m_appsModel;
}

FlatpakReference *AppPermissionsKCM::flatpakRefForApp(const QString &appId)
{
    auto it = std::ranges::find(m_references, appId, &FlatpakReference::flatpakName);
    if (it == std::ranges::end(m_references)) {
        return nullptr;
    }
    QQmlEngine::setObjectOwnership(it->get(), QQmlEngine::CppOwnership);
    return it->get();
}

#include "kcm.moc"

#include "moc_kcm.cpp"
