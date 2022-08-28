/**
 * SPDX-FileCopyrightText: 2022 Suhaas Joshi <joshiesuhaas0@gmail.com>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

import QtQuick 2.0
import QtQuick.Controls 2.15 as Controls
import QtQuick.Layouts 1.15 as Layouts

import org.kde.kcm 1.2 as KCM
import org.kde.kirigami 2.19 as Kirigami
import org.kde.plasma.kcm.flatpakpermissions 1.0

KCM.ScrollViewKCM {
    id: basicPermissionPage
    title: i18n("Basic Permissions")
    implicitWidth: Kirigami.Units.gridUnit * 10
    framedView: true
    property QtObject permsModel

    view: ListView {
        id: permsView
        model: permsModel
        currentIndex: -1
        delegate: PermissionDelegate {
            showAdvanced: false
        }
    }
}
