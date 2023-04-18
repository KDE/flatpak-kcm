/**
 * SPDX-FileCopyrightText: 2022 Suhaas Joshi <joshiesuhaas0@gmail.com>
 * SPDX-FileCopyrightText: 2023 ivan tkachenko <me@ratijas.tk>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include "flatpakpermission.h"

#include <QString>
#include <QVector>

namespace FlatpakHelper
{

QString permDataFilePath();

QString iconPath(const QString &name, const QString &id, const QString &appBasePath);

// Port of flatpak_verify_dbus_name static/internal function.
bool verifyDBusName(QStringView name);
}
