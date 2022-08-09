#pragma once

#include "flatpakpermission.h"

#include <QString>
#include <QVector>

namespace FlatpakHelper
{
QString permDataFilePath();
QString iconPath(const QString &name, const QString &id);
}
