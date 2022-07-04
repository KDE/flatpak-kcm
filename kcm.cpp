/**
 * SPDX-FileCopyrightText: 2022 Suhaas Joshi <joshiesuhaas0@gmail.com>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kcm.h"

#include <KPluginFactory>
#include <KLocalizedString>
#include <QFile>

K_PLUGIN_CLASS_WITH_JSON(KCMFlatpak, "kcm_flatpak.json")

KCMFlatpak::KCMFlatpak(QObject *parent, const KPluginMetaData &data, const QVariantList &args)
    : KQuickAddons::ManagedConfigModule(parent, data, args),
      m_refsModel(new FlatpakReferencesModel(this))
{
    qmlRegisterUncreatableType<KCMFlatpak>("org.kde.plasma.kcm.flatpakpermissions", 1, 0, "KCMFlatpak", QString());
}

void KCMFlatpak::editPerm(QString path, QString name, bool isGranted, QString category, QString defaultValue)
{
    QFile inF(path);

    bool settingToNonDefault = (!isGranted && defaultValue == QStringLiteral("OFF")) || (isGranted && defaultValue == QStringLiteral("ON"));
    if(!settingToNonDefault && !inF.exists()) {
        return;
    }

    if(!inF.open(QIODevice::ReadOnly)) {
        qInfo() << "Not opening";
    }

    QTextStream instream(&inF);
    QString data = instream.readAll();
    inF.close();

    if(isGranted && defaultValue == QStringLiteral("OFF")) {
        /* we must un-grant */
        int indexPerm = data.indexOf(name);
        int indexCat = data.indexOf(category);
        /* count no. of permissions. if only 1, delete the entire category entry */
        int permCount = 0, i;
        for(i = indexCat; data[i] != QLatin1Char('\n'); ++i) {
            if(data[i] == QLatin1Char(';')) {
                permCount++;
            }
        }

        if(indexPerm != -1) {
            if(permCount == 0) {
                /* delete entire cat entry, incl \n at the end */
                data.remove(indexCat, i - indexCat + 1);
            } else {
                if(data[data.indexOf(name) + name.length()] == QLatin1Char(';')) {

                    data.remove(indexPerm, name.length() + 1);
                } else {

                    /* remove permission, incl ; of the previous permission */
                    data.remove(indexPerm - 1, name.length() + 1);
                }
            }
        }
    } else if(!isGranted && defaultValue == QStringLiteral("OFF")) {
        if(!data.contains(QStringLiteral("[Context]"))) {
            data.insert(0, QStringLiteral("[Context]"));
        }
        if(data.contains(category)) {
            int index = data.indexOf(QLatin1Char('='), data.indexOf(category));
            data.insert(index + 1, name.append(QLatin1Char(';')));
        } else {
            data.push_back(name.prepend(QLatin1Char('\n') + category + QLatin1Char('=')));
        }
    } else if(isGranted && defaultValue == QStringLiteral("ON")) {
        if(!data.contains(QStringLiteral("[Context]"))) {
            data.insert(0, QStringLiteral("[Context]"));
        }
        if(data.contains(category)) {
            int index = data.indexOf(QLatin1Char('='), data.indexOf(category));
            data.insert(index + 1, name.prepend(QLatin1Char('!')).append(QLatin1Char(';')));
        } else {
            data.push_back(name.prepend(QLatin1Char('\n') + category + QStringLiteral("=!")));
        }
    } else if(inF.exists() && !isGranted && defaultValue == QStringLiteral("ON")) {
        /* we must un-grant */
        int indexPerm = data.indexOf(name);
        int indexCat = data.indexOf(category);
        /* count no. of permissions. if only 1, delete the entire category entry */
        int permCount = 0, i;
        for(i = indexCat; data[i] != QLatin1Char('\n'); ++i) {
            if(data[i] == QLatin1Char(';')) {
                permCount++;
            }
        }

        if(indexPerm != -1) {
            if(permCount == 0) {
                /* delete entire cat entry, incl \n at the end */
                data.remove(indexCat, i - indexCat + 1);
            } else {
                if(data[data.indexOf(name) + name.length()] == QLatin1Char(';')) {
                    data.remove(indexPerm - 1, name.length() + 2);
                } else {

                    /* remove permission, incl ; of the previous permission */
                    data.remove(indexPerm - 2, name.length() + 2);
                }
            }
        }
    }

    QFile outF(path);

    if(!outF.open(QIODevice::WriteOnly)) {
        qInfo() << "WRITE NOT OPENED";
    }

    QTextStream out(&outF);
    out << data;

    outF.close();
}

#include "kcm.moc"
