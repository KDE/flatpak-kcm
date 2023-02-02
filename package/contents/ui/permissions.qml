/**
 * SPDX-FileCopyrightText: 2022 Suhaas Joshi <joshiesuhaas0@gmail.com>
 * SPDX-FileCopyrightText: 2023 ivan tkachenko <me@ratijas.tk>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

import QtQuick 2.15
import QtQuick.Controls 2.15 as QQC2
import QtQuick.Layouts 1.15

import org.kde.kcm 1.2 as KCM
import org.kde.kirigami 2.20 as Kirigami
import org.kde.plasma.kcm.flatpakpermissions 1.0

KCM.ScrollViewKCM {
    id: permissionPage
    title: i18n("Permissions")
    implicitWidth: Kirigami.Units.gridUnit * 15
    framedView: false
    property var ref: null
    property bool showAdvanced: true

    Kirigami.PlaceholderMessage {
        text: i18n("Select an application from the list to view its permissions here")
        width: parent.width - (Kirigami.Units.largeSpacing * 4)
        anchors.centerIn: parent
        visible: ref === null
    }

    Kirigami.Separator {
        anchors.left: parent.left
        height: parent.height
    }

    view: ListView {
        id: permsView
        model: FlatpakPermissionModel {
            id: permsModel
            reference: permissionPage.ref
        }

        currentIndex: -1

        section.property: permissionPage.showAdvanced ? "category" : "sectionType"
        section.criteria: ViewSection.FullString
        section.delegate: Kirigami.ListSectionHeader {
            label: section
            QQC2.ToolButton {
                id: buttonToggleAdvanced
                text: permissionPage.showAdvanced ? i18n("Hide advanced permissions") : i18n("Show advanced permissions")
                display: QQC2.AbstractButton.IconOnly
                icon.name: permissionPage.showAdvanced ? "collapse" : "expand"
                visible: section === i18n("Advanced Permissions")
                onClicked: permissionPage.showAdvanced = !permissionPage.showAdvanced
                Layout.alignment: Qt.AlignRight

                QQC2.ToolTip.text: text
                QQC2.ToolTip.visible: Kirigami.Settings.tabletMode ? buttonToggleAdvanced.pressed : buttonToggleAdvanced.hovered
                QQC2.ToolTip.delay: Kirigami.Settings.tabletMode ? Qt.styleHints.mousePressAndHoldInterval : Kirigami.Units.toolTipDelay
            }
            QQC2.ToolButton {
                id: buttonAddNew
                text: i18n("Add New")
                icon.name: "bqm-add"
                visible: [
                    i18n("Filesystem Access"),
                    i18n("Environment"),
                    i18n("Session Bus Policy"),
                    i18n("System Bus Policy"),
                ].includes(section)
                onClicked: {
                    textPromptDialog.open()
                }
                Layout.alignment: Qt.AlignRight

                QQC2.ToolTip.text: switch (section) {
                    case i18n("Filesystem Access"): return i18n("Add a new filesystem path")
                    case i18n("Environment"): return i18n("Add a new environment variable")
                    case i18n("Session Bus Policy"): return i18n("Add a new session bus")
                    default: return i18n("Add a new system bus")
                }
                QQC2.ToolTip.visible: Kirigami.Settings.tabletMode ? buttonAddNew.pressed : buttonAddNew.hovered
                QQC2.ToolTip.delay: Kirigami.Settings.tabletMode ? Qt.styleHints.mousePressAndHoldInterval : Kirigami.Units.toolTipDelay

                Kirigami.PromptDialog {
                    id: textPromptDialog
                    parent: permissionPage
                    title: switch (section) {
                        case i18n("Filesystem Access"): return i18n("Add Filesystem Path Permission")
                        case i18n("Environment"): return i18n("Set Environment Variable")
                        case i18n("Session Bus Policy"): return i18n("Add Session Bus Permission")
                        default: return i18n("Add System Bus Permission")
                    }

                    RowLayout {
                        QQC2.TextField {
                            id: nameField
                            placeholderText: switch (section) {
                                case i18n("Filesystem Access"): return i18n("Enter filesystem path...")
                                case i18n("Environment"): return i18n("Enter variable...")
                                case i18n("Session Bus Policy"): return i18n("Enter session bus name...")
                                default: return i18n("Enter system bus name...")
                            }
                            Layout.fillWidth: true
                        }
                        QQC2.ComboBox {
                            id: valueBox
                            model: permsModel.valueList(section)
                            visible: section !== i18n("Environment")
                            Layout.fillWidth: true
                        }
                        QQC2.TextField {
                            id: valueField
                            visible: section === i18n("Environment")
                            placeholderText: i18n("Enter value...")
                            Layout.fillWidth: true
                        }
                    }
                    property string value: section === i18n("Environment") ? valueField.text : valueBox.currentText
                    standardButtons: Kirigami.Dialog.Ok | Kirigami.Dialog.Cancel
                    onAccepted: permsModel.addUserEnteredPermission(nameField.text, section, textPromptDialog.value)
                }
            }
        }

        /* FIXME: use Kirigami.CheckableListItem here. Currently it uses BasicListItem, because
         * clicking the checkbox in CheckableListItem does not call the associated slot, and by implication
         * does not enable the "apply" and "reset" buttons. Once a solution for this has been found,
         * the delegate below must be ported to CheckableListItem.
         */
        delegate: Kirigami.BasicListItem {
            id: permItem

            property bool isComplex: !(model.isSimple) && !(model.isEnvironment)
            property var comboVals: model.valueList
            property int index: model.index

            text: model.description
            visible: showAdvanced ? model.isNotDummy : model.isBasic && model.isNotDummy
            height: Kirigami.Units.gridUnit * 2
            hoverEnabled: false
            checkable: true
            activeBackgroundColor: "transparent"
            activeTextColor: Kirigami.Theme.textColor
            onClicked: permsModel.setPerm(permItem.index)

            leading: QQC2.CheckBox {
                id: checkBox
                checked: model.isGranted
                onToggled: permsModel.setPerm(permItem.index)
            }

            trailing: RowLayout {
                QQC2.ComboBox {
                    enabled: checkBox.checked
                    model: permItem.comboVals
                    visible: permItem.isComplex
                    onActivated: index => permsModel.editPerm(permItem.index, textAt(index))
                }
                QQC2.TextField {
                    text: model.currentValue
                    visible: model.isEnvironment
                    enabled: checkBox.checked
                    Keys.onReturnPressed: permsModel.editPerm(permItem.index, text)
                }
            }
        }
    }
}
