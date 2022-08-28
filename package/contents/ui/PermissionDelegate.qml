/**
 * SPDX-FileCopyrightText: 2022 Suhaas Joshi <joshiesuhaas0@gmail.com>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

import QtQuick 2.0
import QtQuick.Controls 2.15 as Controls
import QtQuick.Layouts 1.15 as Layouts

import org.kde.kirigami 2.19 as Kirigami

Kirigami.CheckableListItem {
    id: permItem
    property bool showAdvanced

    text: model.description
    checked: model.isGranted
    visible: showAdvanced ? model.isNotDummy && !model.isBasic : model.isBasic && model.isNotDummy
    height: Kirigami.Units.gridUnit * 2

    onClicked: permsModel.setPerm(permsView.currentIndex, model.isGranted)

    property bool isComplex: !(model.isSimple) && !(model.isEnvironment)
    property var comboVals: model.valueList
    property int index: model.index

    trailing: Layouts.RowLayout {
        Controls.ComboBox {
            enabled: permItem.checked
            model: permItem.comboVals
            visible: permItem.isComplex
            height: Kirigami.Units.gridUnit * 0.5
            onActivated: (index) =>permsModel.editPerm(permItem.index, textAt(index))
        }
        Controls.TextField {
            text: model.currentValue
            visible: model.isEnvironment
            enabled: permItem.checked
            Keys.onReturnPressed: permsModel.editPerm(permItem.index, text)
        }
    }
}
