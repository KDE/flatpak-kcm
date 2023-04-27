/**
 * SPDX-FileCopyrightText: 2022 Suhaas Joshi <joshiesuhaas0@gmail.com>
 * SPDX-FileCopyrightText: 2023 ivan tkachenko <me@ratijas.tk>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <QString>
#include <QUrl>
#include <QVector>

namespace FlatpakHelper
{

QString permissionsDataDirectory();

QUrl iconSourceUrl(const QString &displayName, const QString &flatpakName, const QString &appBasePath);

// Port of flatpak_verify_dbus_name static/internal function.
bool verifyDBusName(QStringView name);
}
