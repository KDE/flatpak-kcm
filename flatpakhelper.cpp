/**
 * SPDX-FileCopyrightText: 2022 Suhaas Joshi <joshiesuhaas0@gmail.com>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "flatpakhelper.h"

#include <QDir>
#include <QFileInfo>

namespace FlatpakHelper
{
QString permDataFilePath()
{
    QString userPath = QString::fromStdString(qgetenv("FLATPAK_USER_DIR").toStdString());
    if(userPath.isEmpty()) {
        userPath = QString::fromStdString(qgetenv("HOST_XDG_DATA_HOME").toStdString());
        if(userPath.isEmpty()) {
            userPath = QDir::homePath().append(QStringLiteral("/.local/share"));
        }
    }
    userPath.append(QStringLiteral("/flatpak"));
    QString overrides = QStringLiteral("overrides");

    QDir dir(userPath);
    if (!dir.exists(overrides)) {
        dir.mkdir(overrides);
    }
    dir.cd(overrides);

    return dir.absolutePath().append(QLatin1Char('/'));
}

QString iconPath(const QString &name, const QString &id)
{
    QString dirPath = QStringLiteral("/var/lib/flatpak/app/") + id + QStringLiteral("/current/active/files/share/icons/hicolor/");
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

}
