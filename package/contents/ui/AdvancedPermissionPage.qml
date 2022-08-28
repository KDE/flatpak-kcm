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
    property QtObject permsModel

    view: ListView {
        id: permsView
        model: permsModel
        currentIndex: -1

        section.property: "category"
        section.criteria: ViewSection.FullString
        section.delegate: Kirigami.ListSectionHeader {
            label: section
            font.bold: true
            height: Kirigami.Units.gridUnit * 2.5
            Controls.Button {
                text: i18n("Add New")
                icon.name: "bqm-add"
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
                            return i18n("Add a new filesystem path")
                        } else if (label === i18n("Environment")) {
                            return i18n("Add a new environment variable")
                        } else if (label === i18n("Session Bus Policy")){
                            return i18n("Add a new session bus")
                        } else {
                            return i18n("Add a new system bus")
                        }
                    }
                    text: getToolTipText()
                }

                Kirigami.PromptDialog {
                    id: textPromptDialog
                    title: getDialogTitle()
                    parent: permissionPage

                    function getPlaceHolderText()
                    {
                        if (label === i18n("Filesystem Access")) {
                            return i18n("Enter filesystem path...")
                        } else if (label === i18n("Environment")) {
                            return i18n("Enter variable...")
                        } else if (label === i18n("Session Bus Policy")){
                            return i18n("Enter session bus name...")
                        } else {
                            return i18n("Enter system bus name...")
                        }
                    }

                    function getDialogTitle()
                    {
                        if (label === i18n("Filesystem Access")) {
                            return i18n("Add Filesystem Path Permission")
                        } else if (label === i18n("Environment")) {
                            return i18n("Set Environment Variable")
                        } else if (label === i18n("Session Bus Policy")){
                            return i18n("Add Session Bus Permission")
                        } else {
                            return i18n("Add System Bus Permission")
                        }
                    }

                    Layouts.RowLayout {
                        Controls.TextField {
                            id: nameField
                            placeholderText: textPromptDialog.getPlaceHolderText()
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

        delegate: PermissionDelegate {
            showAdvanced: true
        }
    }
}
