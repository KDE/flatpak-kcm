/**
 * SPDX-FileCopyrightText: 2022 Suhaas Joshi <joshiesuhaas0@gmail.com>
 * SPDX-FileCopyrightText: 2023 ivan tkachenko <me@ratijas.tk>
 * SPDX-FileCopyrightText: 2025 David Redondo <kde@david-redondo.de>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include "appsmodel.h"
#include "flatpakreference.h"

#include <KQuickManagedConfigModule>

class PermissionStore;

class AppPermissionsKCM : public KQuickManagedConfigModule
{
    Q_OBJECT
    Q_PROPERTY(const AppsModel *appsModel READ appsModel CONSTANT)
    Q_PROPERTY(bool gamemodeAvailable READ gamemodeAvailable CONSTANT)
public:
    /**
     * This KCM manages permissions for Flatpak application. It can open any
     * installed application page directly: use @param args in the form of
     * ["<ref>"], where <ref> is a FlatpakRef in formatted like "app/org.videolan.VLC/x86_64/stable".
     */
    explicit AppPermissionsKCM(QObject *parent, const KPluginMetaData &data, const QVariantList &args);

    const AppsModel *appsModel() const;

    Q_INVOKABLE FlatpakReference *flatpakRefForApp(const QString &appId);
    bool gamemodeAvailable() const;

public Q_SLOTS:
    void load() override;
    void save() override;
    void defaults() override;

private:
    FlatpakReference *indexFromArgs(const QVariantList &args) const;

    AppsModel m_appsModel;
    std::vector<std::unique_ptr<FlatpakReference>> m_references;
    std::shared_ptr<PermissionStore> m_permissionStore;
};
