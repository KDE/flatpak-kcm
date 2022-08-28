/**
 * SPDX-FileCopyrightText: 2022 Suhaas Joshi <joshiesuhaas0@gmail.com>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

import QtQuick 2.0

import org.kde.kirigami 2.19 as Kirigami
import org.kde.kcm 1.2 as KCM
import org.kde.plasma.kcm.flatpakpermissions 1.0

KCM.SimpleKCM {
    id: basicAdvOptions
    Kirigami.ColumnView.fillWidth: false
    implicitWidth: Kirigami.Units.gridUnit * 10
    implicitHeight: Kirigami.Units.gridUnit * 20
    property var ref
    property bool toPop: false

    FlatpakPermissionModel {
        id: permsModel
        reference: ref
    }

    ListView {
        currentIndex: -1
        model: ListModel {
            ListElement {
                optNo: 1
                option: "Basic Permissions"
            }
            ListElement {
                optNo: 2
                option: "Advanced Permissions"
            }
        }
        delegate: Kirigami.BasicListItem {
            text: model.option
            function shouldPop()
            {
                if(toPop === false) {
                    toPop = true
                } else {
                    kcm.pop()
                }
            }
            onClicked: {
                shouldPop()
                var page = model.optNo === 1 ? "BasicPermissionPage.qml" : "AdvancedPermissionPage.qml"
                kcm.push(page, {permsModel: permsModel})
            }
        }
    }
}
