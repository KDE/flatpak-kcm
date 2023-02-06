/**
 * SPDX-FileCopyrightText: 2022 Suhaas Joshi <joshiesuhaas0@gmail.com>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include "flatpakreference.h"

#include <KQuickAddons/ManagedConfigModule>

class KCMFlatpak : public KQuickAddons::ManagedConfigModule
{
    Q_OBJECT
    Q_PROPERTY(FlatpakReferencesModel *refsModel MEMBER m_refsModel CONSTANT)
public:
    /**
     * This KCM manages permissions for Flatpak application. It can open any
     * installed application page directly: use @param args in the form of
     * ["<ref>"], where <ref> is a FlatpakRef in formatted like "app/org.videolan.VLC/x86_64/stable".
     */
    explicit KCMFlatpak(QObject *parent, const KPluginMetaData &data, const QVariantList &args);

    void refreshSaveNeeded();

public Q_SLOTS:
    void load() override;
    void save() override;
    void defaults() override;
    bool isSaveNeeded() const override;
    bool isDefaults() const override;

public Q_SLOTS:
    void setIndex(int index);
    int currentIndex() const;

private:
    FlatpakReferencesModel *m_refsModel;
    int m_index = -1;
};
