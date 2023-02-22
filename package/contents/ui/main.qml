/**
 * SPDX-FileCopyrightText: 2022 Suhaas Joshi <joshiesuhaas0@gmail.com>
 * SPDX-FileCopyrightText: 2023 ivan tkachenko <me@ratijas.tk>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15 as QQC2
import org.kde.kirigami 2.20 as Kirigami
import org.kde.kcm 1.2 as KCM
import org.kde.kitemmodels 1.0 as KItemModels
import org.kde.plasma.kcm.flatpakpermissions 1.0

KCM.ScrollViewKCM {
    id: root
    title: i18n("Flatpak Applications")
    Kirigami.ColumnView.fillWidth: false
    implicitWidth: Kirigami.Units.gridUnit * 40
    implicitHeight: Kirigami.Units.gridUnit * 20
    framedView: false

    function shouldChange(toAppAtFilteredRowIndex: int) {
        const fromAppAtSourceRowIndex = kcm.currentIndex();

        const toAppAtSourceRowIndex = (toAppAtFilteredRowIndex === -1)
            ? toAppAtFilteredRowIndex
            : filteredRefsModel.mapToSource(filteredRefsModel.index(toAppAtFilteredRowIndex, 0)).row;

        if (toAppAtSourceRowIndex === kcm.currentIndex()) {
            // Don't reload if it's current anyway.
            return;
        }

        if (fromAppAtSourceRowIndex !== -1 && kcm.isSaveNeeded()) {
            const m = kcm.refsModel;
            const fromAppAtSourceIndex = m.index(fromAppAtSourceRowIndex, 0);
            const applicationName = m.data(fromAppAtSourceIndex, FlatpakReferencesModel.Name);
            const applicationIcon = m.data(fromAppAtSourceIndex, FlatpakReferencesModel.Icon);
            const dialog = applyOrDiscardDialogComponent.createObject(this, {
                applicationName,
                applicationIcon,
                fromAppAtSourceRowIndex,
                toAppAtSourceRowIndex,
            });
            dialog.open();
        } else {
            changeApp(toAppAtSourceRowIndex)
        }
    }

    function changeApp(toAppAtSourceRowIndex) {
        let ref = null;
        if (toAppAtSourceRowIndex !== -1) {
            const sourceIndex = kcm.refsModel.index(toAppAtSourceRowIndex, 0);
            ref = kcm.refsModel.data(sourceIndex, FlatpakReferencesModel.Ref);
            appsListView.setCurrentIndexLater(toAppAtSourceRowIndex);
        }
        kcm.pop(0);
        kcm.setIndex(toAppAtSourceRowIndex);
        kcm.push("permissions.qml", { ref });
    }

    Component.onCompleted: {
        kcm.columnWidth = Kirigami.Units.gridUnit * 15
        changeApp(kcm.currentIndex());
    }

    KCM.ConfigModule.buttons: KCM.ConfigModule.Apply | KCM.ConfigModule.Default

    Component {
        id: applyOrDiscardDialogComponent

        Kirigami.PromptDialog {
            id: dialog

            required property string applicationName
            required property url applicationIcon

            // source model indices
            required property int fromAppAtSourceRowIndex
            required property int toAppAtSourceRowIndex

            readonly property bool narrow: (parent.width - leftMargin - rightMargin) < Kirigami.Units.gridUnit * 20

            parent: root.Kirigami.ColumnView.view
            title: i18n("Apply Permissions")
            subtitle: i18n("The permissions of application %1 have been changed. Do you want to apply these changes or discard them?", applicationName)
            standardButtons: QQC2.Dialog.Apply | QQC2.Dialog.Discard

            GridLayout {
                columns: dialog.narrow ? 1 : 2
                columnSpacing: Kirigami.Units.largeSpacing
                rowSpacing: Kirigami.Units.largeSpacing

                Kirigami.Icon {
                    source: dialog.applicationIcon

                    Layout.alignment: Qt.AlignCenter
                    Layout.preferredWidth: Kirigami.Units.iconSizes.large
                    Layout.preferredHeight: Kirigami.Units.iconSizes.large
                }
                Kirigami.SelectableLabel {
                    text: dialog.subtitle
                    wrapMode: Text.Wrap

                    Layout.fillWidth: true
                    Layout.fillHeight: true
                }
            }

            QQC2.Overlay.modal: KcmPopupModal {}

            onOpened: {
                const button = standardButton(QQC2.Dialog.Apply);
                button.forceActiveFocus(Qt.TabFocusReason);
            }

            onApplied: {
                kcm.save()
                root.changeApp(toAppAtSourceRowIndex)
                dialog.close()
            }

            onDiscarded: {
                kcm.load()
                root.changeApp(fromAppAtSourceRowIndex)
                dialog.close()
            }

            onRejected: {
                appsListView.currentIndex = kcm.currentIndex()
            }

            onClosed: destroy()
        }
    }

    header: Kirigami.SearchField {
        id: filterField
        KeyNavigation.tab: appsListView
        KeyNavigation.down: appsListView
        autoAccept: false
        onAccepted: {
            if (text === "") {
                // The signal also fires when user clicks the "Clear" action/icon/button.
                // In this case don't treat it as a command to open first app.
                return;
            }
            if (filteredRefsModel.count >= 0) {
                appsListView.setCurrentIndexLater(0);
                root.shouldChange(0);
            }
        }
    }

    view: ListView {
        id: appsListView

        function setCurrentIndexLater(sourceRowIndex) {
            // View has not updated yet, and alas -- we don't have suitable
            // hooks here. Note that ListView::onCountChanged WON'T WORK, as
            // unlike KSortFilterProxyModel it won't trigger when changing
            // from e.g. 2 rows [a, b] to another 2 rows [d, c].

            const sourceIndex = filteredRefsModel.sourceModel.index(sourceRowIndex, 0);
            const filteredIndex = filteredRefsModel.mapFromSource(sourceIndex);
            const filteredRowIndex = filteredIndex.valid ? filteredIndex.row : -1;

            delayedCurrentIndexSetter.index = filteredRowIndex;
            delayedCurrentIndexSetter.start();
        }

        // Using Timer object instead of Qt.callLater to get deduplication for free.
        Timer {
            id: delayedCurrentIndexSetter

            property int index: -1

            interval: 0
            running: false

            onTriggered: {
                appsListView.currentIndex = index;
            }
        }

        model: KItemModels.KSortFilterProxyModel {
            id: filteredRefsModel
            sourceModel: kcm.refsModel
            sortOrder: Qt.AscendingOrder
            sortRole: FlatpakReferencesModel.Name
            filterRole: FlatpakReferencesModel.Name
            filterString: filterField.text
            filterCaseSensitivity: Qt.CaseInsensitive
            onCountChanged: {
                if (count >= 0) {
                    const sourceRowIndex = kcm.currentIndex();
                    appsListView.setCurrentIndexLater(sourceRowIndex);
                }
            }
        }
        currentIndex: -1
        delegate: Kirigami.BasicListItem {

            text: model.name
            icon: model.icon

            onClicked: root.shouldChange(index)
        }
    }
}
