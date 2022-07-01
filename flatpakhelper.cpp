#include "flatpakhelper.h"

#include <QDir>

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
}
