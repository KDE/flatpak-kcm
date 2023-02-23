/**
 * SPDX-FileCopyrightText: 2022 Suhaas Joshi <joshiesuhaas0@gmail.com>
 * SPDX-FileCopyrightText: 2023 ivan tkachenko <me@ratijas.tk>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

import QtQuick 2.15
import QtQuick.Controls 2.15 as QQC2
import QtQuick.Layouts 1.15

import org.kde.kcm 1.6 as KCM
import org.kde.kirigami 2.20 as Kirigami
import org.kde.plasma.kcm.flatpakpermissions 1.0

KCM.ScrollViewKCM {
    id: root
    title: i18n("Permissions")
    implicitWidth: Kirigami.Units.gridUnit * 15
    framedView: false
    property var ref: null

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

    Component {
        id: addEnvironmentVariableDialogComponent

        AddEnvironmentVariableDialog {
            parent: root.Kirigami.ColumnView.view
            model: permsModel

            onClosed: destroy()
        }
    }

    Component {
        id: textPromptDialogComponent

        TextPromptDialog {
            parent: root.Kirigami.ColumnView.view
            model: permsModel

            onClosed: destroy()
        }
    }

    view: ListView {
        model: FlatpakPermissionModel {
            id: permsModel
            reference: root.ref
        }

        currentIndex: -1
        // Mitigate ListView's poor layouting stills.
        // Without it, section delegates may shift down and overlap.
        reuseItems: false
        cacheBuffer: 10000

        section.property: "section"
        section.criteria: ViewSection.FullString
        section.delegate: Kirigami.ListSectionHeader {
            id: sectionDelegate

            /**
             * Sorry about this mess. ListView automatically converts data of
             * section role to string, so we parse it back into enum.
             */
            property /*FlatpakPermissionsSectionType::Type*/ int sectionType: parseInt(section)

            width: ListView.view.width - ListView.view.leftMargin - ListView.view.rightMargin
            label: permsModel.sectionHeaderForSectionType(sectionType)
            QQC2.ToolButton {
                text: permsModel.showAdvanced ? i18n("Hide advanced permissions") : i18n("Show advanced permissions")
                display: QQC2.AbstractButton.IconOnly
                icon.name: permsModel.showAdvanced ? "collapse" : "expand"
                visible: sectionDelegate.sectionType === FlatpakPermissionsSectionType.Advanced
                onClicked: permsModel.showAdvanced = !permsModel.showAdvanced
                Layout.alignment: Qt.AlignRight

                QQC2.ToolTip.text: text
                QQC2.ToolTip.visible: Kirigami.Settings.tabletMode ? pressed : hovered
                QQC2.ToolTip.delay: Kirigami.Settings.tabletMode ? Qt.styleHints.mousePressAndHoldInterval : Kirigami.Units.toolTipDelay
            }
            QQC2.ToolButton {
                text: i18n("Add New")
                icon.name: "bqm-add"
                visible: [
                    FlatpakPermissionsSectionType.Filesystems,
                    FlatpakPermissionsSectionType.SessionBus,
                    FlatpakPermissionsSectionType.SystemBus,
                    FlatpakPermissionsSectionType.Environment,
                ].includes(sectionDelegate.sectionType)
                onClicked: {
                    if (sectionDelegate.sectionType === FlatpakPermissionsSectionType.Environment) {
                        const dialog = addEnvironmentVariableDialogComponent.createObject(root, {
                            model: permsModel,
                        });
                        dialog.open();
                    } else {
                        const dialog = textPromptDialogComponent.createObject(root, {
                            model: permsModel,
                            sectionType: sectionDelegate.sectionType,
                        });
                        dialog.open();
                    }
                }
                Layout.alignment: Qt.AlignRight

                QQC2.ToolTip.text: permsModel.sectionAddButtonToolTipTextForSectionType(sectionDelegate.sectionType)
                QQC2.ToolTip.visible: Kirigami.Settings.tabletMode ? pressed : hovered
                QQC2.ToolTip.delay: Kirigami.Settings.tabletMode ? Qt.styleHints.mousePressAndHoldInterval : Kirigami.Units.toolTipDelay
            }
        }

        /* FIXME: use Kirigami.CheckableListItem here. Currently it uses BasicListItem, because
         * clicking the checkbox in CheckableListItem does not call the associated slot, and by implication
         * does not enable the "apply" and "reset" buttons. Once a solution for this has been found,
         * the delegate below must be ported to CheckableListItem.
         */
        delegate: Kirigami.BasicListItem {
            id: permItem

            required property var model
            required property int index

            // Default-provided custom entries are not meant to be unchecked:
            // it is a meaningless undefined operation.
            checkable: model.canBeDisabled

            // default formula does not take leading/trailing into account
            implicitHeight: Math.max(iconSize, labelItem.implicitHeight, trailing.implicitHeight) + topPadding + bottomPadding
            width: ListView.view.width - ListView.view.leftMargin - ListView.view.rightMargin
            height: implicitHeight
            trailingFillVertically: false

            text: model.description
            visible: model.isNotDummy

            onClicked: {
                if (checkable) {
                    permsModel.togglePermissionAtRow(permItem.index);
                }
                permItem.ListView.view.currentIndex = -1;
            }

            leading: QQC2.CheckBox {
                id: checkBox
                enabled: permItem.checkable
                checked: permItem.model.isEffectiveEnabled

                onToggled: {
                    permsModel.togglePermissionAtRow(permItem.index);
                    permItem.ListView.view.currentIndex = -1;
                }

                KCM.SettingHighlighter {
                    highlight: permItem.model.isEffectiveEnabled !== permItem.model.isDefaultEnabled
                }
            }

            trailing: RowLayout {
                enabled: checkBox.checked
                QQC2.ComboBox {
                    visible: [
                        FlatpakPermissionsSectionType.Filesystems,
                        FlatpakPermissionsSectionType.SessionBus,
                        FlatpakPermissionsSectionType.SystemBus,
                    ].includes(permItem.model.section)

                    enabled: checkBox.checked

                    model: permItem.model.valuesModel
                    textRole: "display"
                    valueRole: "value"

                    KCM.SettingHighlighter {
                        highlight: permItem.model.effectiveValue !== permItem.model.defaultValue
                    }

                    onActivated: index => {
                        // Assuming this is only called for appropriate visible entries.
                        permsModel.setPermissionValueAtRow(permItem.index, currentValue);
                    }

                    Component.onCompleted: {
                        // Still need to check section type, as this is called for every entry.
                        if (permItem.model.isNotDummy && [
                                FlatpakPermissionsSectionType.Filesystems,
                                FlatpakPermissionsSectionType.SessionBus,
                                FlatpakPermissionsSectionType.SystemBus,
                            ].includes(permItem.model.section)) {
                            currentIndex = Qt.binding(() => indexOfValue(permItem.model.effectiveValue));
                        }
                    }
                }

                QQC2.TextField {
                    visible: permItem.model.isNotDummy && permItem.model.section === FlatpakPermissionsSectionType.Environment
                    text: (permItem.model.isNotDummy && permItem.model.section === FlatpakPermissionsSectionType.Environment)
                        ? permItem.model.effectiveValue : ""
                    enabled: checkBox.checked

                    onTextEdited: {
                        permsModel.setPermissionValueAtRow(permItem.index, text);
                    }

                    KCM.SettingHighlighter {
                        highlight: permItem.model.effectiveValue !== permItem.model.defaultValue
                    }
                }
            }
        }
    }
}
