/**
 * SPDX-FileCopyrightText: 2022 Suhaas Joshi <joshiesuhaas0@gmail.com>
 * SPDX-FileCopyrightText: 2023 ivan tkachenko <me@ratijas.tk>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "flatpakhelper.h"

#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>

#include <gio/gio.h>

namespace FlatpakHelper
{

QString permDataFilePath()
{
    QString userPath = QString::fromStdString(qgetenv("FLATPAK_USER_DIR").toStdString());
    if (userPath.isEmpty()) {
        userPath = QString::fromStdString(qgetenv("HOST_XDG_DATA_HOME").toStdString());
        if (userPath.isEmpty()) {
            userPath = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
        }
    }
    userPath.append(QStringLiteral("/flatpak/overrides/"));
    QDir().mkpath(userPath);
    return userPath;
}

QString iconPath(const QString &name, const QString &id, const QString &appBasePath)
{
    QString dirPath = appBasePath + QStringLiteral("/files/share/icons/hicolor/");
    QDir dir(dirPath);
    dir.setFilter(QDir::NoDotAndDotDot | QDir::AllDirs);

    QString nextDir;
    if (dir.exists(QStringLiteral("scalable"))) {
        nextDir = QStringLiteral("scalable");
    } else if (dir.exists(QStringLiteral("symbolic"))) {
        nextDir = QStringLiteral("symbolic");
    } else if (!dir.isEmpty()) {
        nextDir = dir.entryList().at(0);
    } else {
        return QString();
    }
    dir.cd(nextDir + QStringLiteral("/apps"));

    QString file = id + QStringLiteral(".png");
    if (!dir.exists(file)) {
        file = id + QStringLiteral(".svg");
        if (!dir.exists(file)) {
            file = name.toLower() + QStringLiteral(".png");
            if (!dir.exists(file)) {
                file = name.toLower() + QStringLiteral(".svg");
                if (!dir.exists(file)) {
                    return id + QStringLiteral(".png");
                }
            }
        }
    }
    return dir.absoluteFilePath(file);
}

bool verifyDBusName(QStringView name)
{
    if (name.endsWith(QLatin1String(".*"))) {
        name.chop(2);
        name = name.mid(-2);
    }

    const auto ownedName = name.toString();
    const auto arrayName = ownedName.toUtf8();
    const auto cName = arrayName.constData();

    return g_dbus_is_name(cName) && !g_dbus_is_unique_name(cName);
}
}
