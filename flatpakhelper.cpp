#include "flatpakhelper.h"

#include <QDir>
#include <QFileInfo>

namespace FlatpakHelper
{
QString permDataFilePath() {
    QString userPath = QString::fromStdString(qgetenv("FLATPAK_USER_DIR").toStdString());
    if(userPath.isEmpty()) {
        userPath = QString::fromStdString(qgetenv("HOST_XDG_DATA_HOME").toStdString());
        if(userPath.isEmpty()) {
            userPath = QDir::homePath().append(QStringLiteral("/.local/share"));
        }
    }
    userPath.append(QStringLiteral("/flatpak/overrides/"));
    return userPath;
}

QString iconPath(QString name, QString id)
{
    QString dirPath = QStringLiteral("/var/lib/flatpak/app/") + id + QStringLiteral("/current/active/files/share/icons/hicolor/");
    QStringList entries = QDir(dirPath).entryList();
    if (entries.contains(QStringLiteral("scalable"))) {
        dirPath.append(QStringLiteral("scalable/apps/"));
    } else if (entries.contains(QStringLiteral("symbolic"))) {
        dirPath.append(QStringLiteral("symbolic/apps/"));
    } else if (entries.length() > 2) {
        dirPath.append(entries.at(2) + QStringLiteral("/apps/"));
    } else {
        return QString();
    }

    QString filePath = dirPath + id + QStringLiteral(".png");
    if (!QFileInfo::exists(filePath)) {
        filePath = dirPath + id + QStringLiteral(".svg");
        if (!QFileInfo::exists(filePath)) {
            filePath = dirPath + name.toLower() + QStringLiteral(".png");
            if (!QFileInfo::exists(filePath)) {
                filePath = dirPath + name.toLower() + QStringLiteral(".svg");
                if (!QFileInfo::exists(filePath)) {
                    filePath = id + QStringLiteral(".png");
                }
            }
        }
    }
    return filePath;
}

}
