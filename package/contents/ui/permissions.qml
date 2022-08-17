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
    id: permissionPage
    title: i18n("Permissions")
    implicitWidth: Kirigami.Units.gridUnit * 15
    framedView: true
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
            height: Kirigami.Units.gridUnit * 2.5
            Controls.Button {
                text: i18n("Add New")
                visible: label === "Filesystem Access" || label === "Session Bus Policy" || label === "System Bus Policy" || label === "Environment"
                onClicked: {
                    textPromptDialog.open()
                }
                Layouts.Layout.alignment: Qt.AlignRight

                Controls.ToolTip {
                    function getToolTipText()
                    {
                        var toolTipText
                        if (label === i18n("Filesystem Access")) {
                            return i18n("Click here to add a new filesystem path")
                        } else if (label === i18n("Environment")) {
                            return i18n("Click here to add a new environment variable")
                        } else if (label === i18n("Session Bus Policy")){
                            return i18n("Click here to add a new session bus")
                        } else {
                            return i18n("Click here to add a new system bus")
                        }
                    }
                    text: getToolTipText()
                }

                Kirigami.PromptDialog {
                    id: textPromptDialog
                    title: i18n("New Permission Entry")
                    parent: permissionPage
                    Layouts.RowLayout {
                        Controls.TextField {
                            id: nameField
                            placeholderText: i18n("Permission name...")
                            Layouts.Layout.fillWidth: true
                        }
                        Controls.ComboBox {
                            id: valueBox
                            model: permsModel.valueList(section)
                            visible: section !== "Environment"
                            Layouts.Layout.fillWidth: true
                        }
                        Controls.TextField {
                            id: valueField
                            visible: section === "Environment"
                            placeholderText: i18n("Enter value...")
                            Layouts.Layout.fillWidth: true
                        }
                    }
                    property string value: label === "Environment" ? valueField.text : valueBox.currentText
                    standardButtons: Kirigami.Dialog.Ok | Kirigami.Dialog.Close
                    onAccepted: permsModel.addUserEnteredPermission(nameField.text, section, textPromptDialog.value)
                }
            }
        }

        delegate: Kirigami.CheckableListItem {
            id: permItem
            text: model.description
            checked: model.isGranted
            visible: model.isNotDummy
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
