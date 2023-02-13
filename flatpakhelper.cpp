/**
 * SPDX-FileCopyrightText: 2022 Suhaas Joshi <joshiesuhaas0@gmail.com>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "flatpakhelper.h"

#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>
#include <QUrl>

namespace FlatpakHelper
{
QString permissionsDataDirectory()
{
    QString userPath = qEnvironmentVariable("FLATPAK_USER_DIR");
    if (userPath.isEmpty()) {
        userPath = qEnvironmentVariable("HOST_XDG_DATA_HOME");
        if (userPath.isEmpty()) {
            userPath = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
        }
    }
    userPath.append(QStringLiteral("/flatpak/overrides/"));
    QDir().mkpath(userPath);
    return userPath;
}

QUrl iconSourceUrl(const QString &displayName, const QString &flatpakName, const QString &appBasePath)
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
        return QUrl();
    }
    dir.cd(nextDir + QStringLiteral("/apps"));

    QString file = flatpakName + QStringLiteral(".png");
    if (!dir.exists(file)) {
        file = flatpakName + QStringLiteral(".svg");
        if (!dir.exists(file)) {
            file = displayName.toLower() + QStringLiteral(".png");
            if (!dir.exists(file)) {
                file = displayName.toLower() + QStringLiteral(".svg");
                if (!dir.exists(file)) {
                    return QUrl::fromLocalFile(flatpakName + QStringLiteral(".png"));
                }
            }
        }
    }
    return QUrl::fromLocalFile(dir.absoluteFilePath(file));
}

}
