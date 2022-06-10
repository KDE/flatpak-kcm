/**
 * SPDX-FileCopyrightText: 2022 Suhaas Joshi <joshiesuhaas0@gmail.com>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

import QtQuick 2.12
import QtQuick.Controls 2.12 as Controls

import org.kde.kirigami 2.7 as Kirigami
import org.kde.kcm 1.2 as KCM
import org.kde.plasma.kcm.flatpakpermissions 1.0

KCM.ScrollViewKCM {
    id: root
    title: i18n("Flatpak Permissions")
    Kirigami.ColumnView.fillWidth: false
    implicitWidth: Kirigami.Units.gridUnit * 30
    implicitHeight: Kirigami.Units.gridUnit * 20
    framedView: true

    Component.onCompleted: {
        kcm.columnWidth = Kirigami.Units.gridUnit * 15
        kcm.push("permissions.qml")
    }

    view: ListView {
        id: appsListView

        header: Kirigami.Heading {
            text: i18n("Applications")
            level: 1
        }
        model: kcm.refsModel
        currentIndex: -1
        delegate: Kirigami.BasicListItem {
            text: model.name
            subtitle: model.id
            onClicked: {
                kcm.pop()
                kcm.push("permissions.qml")
            }
        }
    }
}

