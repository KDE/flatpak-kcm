/**
 * SPDX-FileCopyrightText: 2022 Suhaas Joshi <joshiesuhaas0@gmail.com>
 * SPDX-FileCopyrightText: 2023 ivan tkachenko <me@ratijas.tk>
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
    explicit KCMFlatpak(QObject *parent, const KPluginMetaData &data, const QVariantList &args);

    Q_INVOKABLE bool isSaveNeeded() const override;
    Q_INVOKABLE bool isDefaults() const override;
    Q_INVOKABLE int currentIndex() const;

public Q_SLOTS:
    void load() override;
    void save() override;
    void defaults() override;
    void setIndex(int index);

private:
    FlatpakReferencesModel *m_refsModel;
    int m_index = -1;
};
