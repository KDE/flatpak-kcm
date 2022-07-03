/**
 * SPDX-FileCopyrightText: 2022 Suhaas Joshi <joshiesuhaas0@gmail.com>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

import QtQuick 2.0
import QtQuick.Controls 2.15 as Controls

import org.kde.kcm 1.2 as KCM
import org.kde.kirigami 2.14 as Kirigami

KCM.ScrollViewKCM {
    id: permissionPage
    title: i18n("Permissions")
    implicitWidth: Kirigami.Units.gridUnit * 15
    property QtObject refModel
    view: ListView {
        model: refModel
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

            //onCheckableChanged: kcm.print(model.name)
            onClicked: kcm.editPerm(model.path, model.name, model.isGranted, model.category, model.defaultValue)

            property bool isComplex: model.isComplex
            property var comboVals: model.valueList

            trailing: Controls.ComboBox {
                enabled: permItem.checked
                model: permItem.comboVals
                visible: permItem.isComplex
                width: Kirigami.Units.gridUnit * 5
            }
        }
    }
}
