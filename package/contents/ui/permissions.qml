/**
 * SPDX-FileCopyrightText: 2022 Suhaas Joshi <joshiesuhaas0@gmail.com>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

import QtQuick 2.0
import QtQuick.Controls 2.15

import org.kde.kcm 1.2 as KCM
import org.kde.kirigami 2.7 as Kirigami

KCM.SimpleKCM {
    id: permissionPage
    title: i18n("Permissions")
    Label {
        text: i18n("Permissions will be displayed here")
    }
    implicitWidth: Kirigami.Units.gridUnit * 15
}
