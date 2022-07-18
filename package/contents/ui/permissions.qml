/**
 * SPDX-FileCopyrightText: 2022 Suhaas Joshi <joshiesuhaas0@gmail.com>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

import QtQuick 2.0
import QtQuick.Controls 2.15 as Controls
import QtQuick.Layouts 1.15 as Layouts

import org.kde.kcm 1.2 as KCM
import org.kde.kirigami 2.14 as Kirigami
import org.kde.plasma.kcm.flatpakpermissions 1.0

KCM.ScrollViewKCM {
    id: permissionPage
    title: i18n("Permissions")
    implicitWidth: Kirigami.Units.gridUnit * 15
    property var ref
    view: ListView {
        id: permsView
        model: FlatpakPermissionModel {
            id: permsModel
            reference: permissionPage.ref
        }

        currentIndex: -1

        section.property: "category"
        section.criteria: ViewSection.FullString
        section.delegate: Kirigami.ListSectionHeader {
            label: section
            font.bold: true
        }

        delegate: Kirigami.CheckableListItem {
            id: permItem
            text: model.description
            checked: model.isGranted

            onClicked: permsModel.setPerm(permsView.currentIndex, model.isGranted)

            property bool isComplex: !(model.isSimple) && !(model.isEnvironment)
            property var comboVals: model.valueList
            property int index: model.index

            trailing: Layouts.RowLayout {
                Controls.ComboBox {
                    enabled: permItem.checked
                    model: permItem.comboVals
                    visible: permItem.isComplex
                    width: Kirigami.Units.gridUnit * 5
                    onActivated: (index) => permsModel.editPerm(permItem.index, textAt(index))
                }
                Controls.TextField {
                    text: model.currentValue
                    visible: model.isEnvironment
                    enabled: permItem.checked
                    Keys.onReturnPressed: permsModel.editPerm(permItem.index, text)
                }
            }
        }
    }
}
