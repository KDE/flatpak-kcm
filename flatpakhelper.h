/**
 * SPDX-FileCopyrightText: 2022 Suhaas Joshi <joshiesuhaas0@gmail.com>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include "flatpakpermission.h"

#include <QString>
#include <QVector>

namespace FlatpakHelper
{
QString permDataFilePath();
QString iconPath(const QString &name, const QString &id);
}
