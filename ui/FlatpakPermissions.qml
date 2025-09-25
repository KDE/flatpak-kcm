/**
 * SPDX-FileCopyrightText: 2022 Suhaas Joshi <joshiesuhaas0@gmail.com>
 * SPDX-FileCopyrightText: 2023 ivan tkachenko <me@ratijas.tk>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts

import org.kde.kcmutils as KCM
import org.kde.kirigami as Kirigami
import org.kde.plasma.kcm.flatpakpermissions

KCM.ScrollViewKCM {
    id: root

    required property var ref
    required property var decoration

    title: i18nc("@title:window %1 is the name of the application" ,"%1 – Flatpak Settings", ref.displayName)
    implicitWidth: Kirigami.Units.gridUnit * 15
    framedView: false

    readonly property bool isCurrentPage: Kirigami.ColumnView.inViewport

    Binding {
        target: kcm
        property: "needsSave"
        value: permsModel.isSaveNeeded
        when: root.isCurrentPage
    }
    Binding {
        target: kcm
        property: "representsDefaults" 
        value: permsModel.isDefaults
        when: root.isCurrentPage
    }



    activeFocusOnTab: true

    Kirigami.Separator {
        anchors.left: parent.left
        height: parent.height
    }

    onActiveFocusChanged: {
        if (activeFocus && ref !== null) {
            view.forceActiveFocus(Qt.TabFocusReason)
        }
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

    // Having it inside a component helps to separate layouts and workarounds for nullable property accesses.
    Component {
        id: headerComponent

        RowLayout {
            id: header

            readonly property string title: root.ref.displayName
            readonly property string subtitle: root.ref.version

            spacing: 0

            width: ListView.view.width - ListView.view.leftMargin - ListView.view.rightMargin

            Kirigami.Icon {
                // Fallback doesn't kick in when source is an empty string/url
                source: root.decoration

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
        model: FlatpakPermissionModel {
            id: permsModel
            reference: root.ref
        }

        currentIndex: -1
        // Mitigate ListView's poor layouting stills.
        // Without it, section delegates may shift down and overlap.
        reuseItems: false
        cacheBuffer: 10000

        Accessible.name: root.ref.displayName
        Accessible.description: i18nc("@info accessible.description for list", "Version %1", root.ref.version)
        Accessible.role: Accessible.List

        activeFocusOnTab: true
        Keys.onBacktabPressed: {
            KCM.ConfigModule.currentIndex = 0
            nextItemInFocusChain(false).forceActiveFocus(Qt.BacktabFocusReason)
        }
        Keys.onTabPressed: event => {
            if (currentIndex === -1) {
                currentIndex = 0
            } else {
                event.accepted = false
            }
        }
        onActiveFocusChanged: if (activeFocus && currentIndex > -1) currentItem.forceActiveFocus(Qt.TabFocusReason)
        KeyNavigation.tab: currentItem?.permComboBox?.activeFocusOnTab ? permComboBox
                         : currentItem?.permTextField?.activeFocusOnTab ? permTextField
                         : nextItemInFocusChain(true)

        // Ref is assumed to remain constant for the lifetime of this page. If
        // it's not null, then it would remain so, and no further checks are
        // needed inside the component.
        header: root.ref === null ? null : headerComponent
        headerPositioning: ListView.InlineHeader

        section.property: "section"
        section.criteria: ViewSection.FullString
        section.delegate: Kirigami.ListSectionHeader {
            id: sectionDelegate

            required property string section

            /**
             * Sorry about this mess. ListView automatically converts data of
             * section role to string, so we parse it back into enum.
             */
            readonly property /*FlatpakPermissionsSectionType::Type*/ int sectionType: parseInt(section)

            width: ListView.view.width - ListView.view.leftMargin - ListView.view.rightMargin
            label: permsModel.sectionHeaderForSectionType(sectionType)
            QQC2.ToolButton {
                text: permsModel.showAdvanced ? i18nc("@action:button in section header", "Hide advanced permissions") : i18nc("@action:button in section header", "Show advanced permissions")
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
                text: i18nc("@action:button in section header", "Add New…")
                Accessible.name: i18nc("action:button accessible", "Add New File System Permission")
                icon.name: "list-add"
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
                Keys.onBacktabPressed: {
                    console.log(root.view.currentItem)
                    root.view.forceActiveFocus(Qt.BacktabFocusReason)
                    root.view.currentItem?.forceActiveFocus(Qt.BacktabFocusReason)

                }


                QQC2.ToolTip.text: permsModel.sectionAddButtonToolTipTextForSectionType(sectionDelegate.sectionType)
                QQC2.ToolTip.visible: Kirigami.Settings.tabletMode ? pressed : hovered
                QQC2.ToolTip.delay: Kirigami.Settings.tabletMode ? Qt.styleHints.mousePressAndHoldInterval : Kirigami.Units.toolTipDelay
            }
        }

        // Can't use a CheckDelegate or one of its subclasses since we need the checkbox
        // to highlight when it's in a non-default state and none of the pre-made delegates
        // can do that
        delegate: QQC2.ItemDelegate {
            id: permItem

            required property var model
            required property int index

            function toggleAndUncheck() {
                if (checkable) {
                    permsModel.togglePermissionAtRow(index);
                }
                ListView.view.currentIndex = -1;
            }

            focus: ListView.isCurrentItem
            width: ListView.view.width - ListView.view.leftMargin - ListView.view.rightMargin

            // Default-provided custom entries are not meant to be unchecked:
            // it is a meaningless undefined operation.
            checkable: model.canBeDisabled
            checked: checkBox.checked

            Accessible.name: model.description

            visible: model.isNotDummy

            onClicked: toggleAndUncheck()
            Keys.onSpacePressed: {
                if (checkable) {
                    permsModel.togglePermissionAtRow(index);
                }
            }

            contentItem: RowLayout {
                spacing: Kirigami.Units.smallSpacing

                QQC2.CheckBox {
                    id: checkBox
                    enabled: permItem.checkable
                    checked: permItem.model.isEffectiveEnabled
                    activeFocusOnTab: false

                    onToggled: permItem.toggleAndUncheck()

                    KCM.SettingHighlighter {
                        highlight: permItem.model.isEffectiveEnabled !== permItem.model.isDefaultEnabled
                    }
                }

                QQC2.Label {
                    Layout.fillWidth: true
                    text: model.description
                    elide: Text.ElideRight
                }

                QQC2.ComboBox {
                    id: permComboBox
                    visible: [
                        FlatpakPermissionsSectionType.Filesystems,
                        FlatpakPermissionsSectionType.SessionBus,
                        FlatpakPermissionsSectionType.SystemBus,
                    ].includes(permItem.model.section)

                    enabled: checkBox.checked
                    activeFocusOnTab: enabled && permItem.ListView.isCurrentItem

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
                    id: permTextField
                    visible: permItem.model.isNotDummy && permItem.model.section === FlatpakPermissionsSectionType.Environment
                    text: (permItem.model.isNotDummy && permItem.model.section === FlatpakPermissionsSectionType.Environment)
                        ? permItem.model.effectiveValue : ""
                    enabled: checkBox.checked
                    activeFocusOnTab: enabled && permItem.ListView.isCurrentItem

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
