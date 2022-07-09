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
    explicit KCMFlatpak(QObject *parent, const KPluginMetaData &data, const QVariantList &args);
    Q_INVOKABLE void editPerm(QString path, QString name, bool isGranted, QString category, QString defaultValue);

private:
    FlatpakReferencesModel *m_refsModel;
};
