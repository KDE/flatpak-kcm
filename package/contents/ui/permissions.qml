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

    // Having it inside a component helps to separate layouts and workarounds for nullable property accesses.
    Component {
        id: headerComponent

        RowLayout {
            id: header

            readonly property url icon: root.ref.iconSource
            readonly property string title: root.ref.displayName
            readonly property string subtitle: root.ref.version

            spacing: 0

            Kirigami.Icon {
                source: header.icon

                Layout.alignment: Qt.AlignCenter
                Layout.preferredWidth: Kirigami.Units.iconSizes.large
                Layout.preferredHeight: Kirigami.Units.iconSizes.large

                // RowLayout is incapable of paddings, so use margins on both child items instead.
                Layout.margins: Kirigami.Units.largeSpacing
            }
            ColumnLayout {
                spacing: Kirigami.Units.smallSpacing
                Layout.fillWidth: true

                Layout.margins: Kirigami.Units.largeSpacing
                Layout.leftMargin: 0

                Kirigami.Heading {
                    text: header.title
                    wrapMode: Text.Wrap
                    Layout.fillWidth: true
                }
                Kirigami.Heading {
                    text: header.subtitle
                    type: Kirigami.Heading.Secondary
                    level: 3
                    wrapMode: Text.Wrap
                    Layout.fillWidth: true
                }
            }
        }
    }

    view: ListView {
        id: permsView
        model: FlatpakPermissionModel {
            id: permsModel
            reference: root.ref
        }

        currentIndex: -1
        // Mitigate ListView's poor layouting stills.
        // Without it, section delegates may shift down and overlap.
        reuseItems: false
        cacheBuffer: 10000

        // Ref is assumed to remain constant for the lifetime of this page. If
        // it's not null, then it would remain so, and no further checks are
        // needed inside the component.
        header: root.ref === null ? null : headerComponent
        headerPositioning: ListView.InlineHeader

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
                id: buttonToggleAdvanced
                text: permsModel.showAdvanced ? i18n("Hide advanced permissions") : i18n("Show advanced permissions")
                display: QQC2.AbstractButton.IconOnly
                icon.name: permsModel.showAdvanced ? "collapse" : "expand"
                visible: sectionDelegate.sectionType === FlatpakPermissionsSectionType.Advanced
                onClicked: permsModel.showAdvanced = !permsModel.showAdvanced
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
                    FlatpakPermissionsSectionType.Filesystems,
                    FlatpakPermissionsSectionType.SessionBus,
                    FlatpakPermissionsSectionType.SystemBus,
                    FlatpakPermissionsSectionType.Environment,
                ].includes(sectionDelegate.sectionType)
                onClicked: {
                    textPromptDialog.open()
                }
                Layout.alignment: Qt.AlignRight

                QQC2.ToolTip.text: permsModel.sectionAddButtonToolTipTextForSectionType(sectionDelegate.sectionType)
                QQC2.ToolTip.visible: Kirigami.Settings.tabletMode ? buttonAddNew.pressed : buttonAddNew.hovered
                QQC2.ToolTip.delay: Kirigami.Settings.tabletMode ? Qt.styleHints.mousePressAndHoldInterval : Kirigami.Units.toolTipDelay

                Kirigami.PromptDialog {
                    id: textPromptDialog
                    parent: root
                    title: switch (sectionDelegate.sectionType) {
                        case FlatpakPermissionsSectionType.Filesystems: return i18n("Add Filesystem Path Permission")
                        case FlatpakPermissionsSectionType.SessionBus: return i18n("Add Session Bus Permission")
                        case FlatpakPermissionsSectionType.SystemBus: return i18n("Add System Bus Permission")
                        case FlatpakPermissionsSectionType.Environment: return i18n("Set Environment Variable")
                        default: return ""
                    }

                    RowLayout {
                        QQC2.TextField {
                            id: nameField
                            placeholderText: switch (sectionDelegate.sectionType) {
                                case FlatpakPermissionsSectionType.Filesystems: return i18n("Enter filesystem path…")
                                case FlatpakPermissionsSectionType.SessionBus: return i18n("Enter session bus name…")
                                case FlatpakPermissionsSectionType.SystemBus: return i18n("Enter system bus name…")
                                case FlatpakPermissionsSectionType.Environment: return i18n("Enter variable…")
                                default: return ""
                            }
                            Layout.fillWidth: true
                        }
                        QQC2.ComboBox {
                            id: valueBox

                            model: permsModel.valuesModelForSectionType(sectionDelegate.sectionType)
                            textRole: "display"
                            valueRole: "value"

                            visible: sectionDelegate.sectionType !== FlatpakPermissionsSectionType.Environment
                            Layout.fillWidth: true
                        }
                        Kirigami.Heading {
                            text: "="
                            level: 3
                            visible: sectionDelegate.sectionType === FlatpakPermissionsSectionType.Environment
                        }
                        QQC2.TextField {
                            id: valueField
                            visible: sectionDelegate.sectionType === FlatpakPermissionsSectionType.Environment
                            placeholderText: i18n("Enter value…")
                            Layout.fillWidth: true
                        }
                    }
                    standardButtons: Kirigami.Dialog.Ok | Kirigami.Dialog.Cancel

                    function getValue(): var {
                        switch (sectionDelegate.sectionType) {
                        case FlatpakPermissionsSectionType.SessionBus:
                        case FlatpakPermissionsSectionType.SystemBus:
                        case FlatpakPermissionsSectionType.Filesystems:
                            return valueBox.currentValue;
                        case FlatpakPermissionsSectionType.Environment:
                            return valueField.text;
                        default:
                            return undefined;
                        }
                    }

                    onAccepted: {
                        const value = getValue();
                        permsModel.addUserEnteredPermission(sectionDelegate.sectionType, nameField.text, value);
                    }
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
            property bool isNotDummy: model.isNotDummy
            property var effectiveValue: model.effectiveValue
            property var valuesModel: model.valuesModel
            property int index: model.index
            property int section: model.section

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
                permsView.currentIndex = -1;
            }

            leading: QQC2.CheckBox {
                id: checkBox
                enabled: permItem.checkable
                checked: model.isEffectiveEnabled
                onToggled: {
                    permsModel.togglePermissionAtRow(permItem.index);
                    permsView.currentIndex = -1;
                }
            }

            trailing: RowLayout {
                QQC2.ComboBox {
                    visible: [
                        FlatpakPermissionsSectionType.Filesystems,
                        FlatpakPermissionsSectionType.SessionBus,
                        FlatpakPermissionsSectionType.SystemBus,
                    ].includes(permItem.section)

                    enabled: checkBox.checked

                    model: permItem.valuesModel
                    textRole: "display"
                    valueRole: "value"

                    onActivated: index => {
                        // Assuming this is only called for appropriate visible entries.
                        permsModel.setPermissionValueAtRow(permItem.index, currentValue);
                    }
                    Component.onCompleted: {
                        // Still need to check section type, as this is called for every entry.
                        if (permItem.isNotDummy && [
                                FlatpakPermissionsSectionType.Filesystems,
                                FlatpakPermissionsSectionType.SessionBus,
                                FlatpakPermissionsSectionType.SystemBus,
                            ].includes(permItem.section)) {
                            currentIndex = Qt.binding(() => indexOfValue(permItem.effectiveValue));
                        }
                    }
                }
                QQC2.TextField {
                    text: model.isEnvironment ? permItem.effectiveValue : ""
                    visible: model.isEnvironment
                    enabled: checkBox.checked
                    Keys.onReturnPressed: {
                        permsModel.setPermissionValueAtRow(permItem.index, text);
                    }
                }
            }
        }
    }
}
