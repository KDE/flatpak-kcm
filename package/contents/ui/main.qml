/**
 * SPDX-FileCopyrightText: 2022 Suhaas Joshi <joshiesuhaas0@gmail.com>
 * SPDX-FileCopyrightText: 2023 ivan tkachenko <me@ratijas.tk>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

import QtQuick 2.15
import QtQuick.Controls 2.15 as QQC2
import org.kde.kirigami 2.20 as Kirigami
import org.kde.kcm 1.2 as KCM
import org.kde.plasma.kcm.flatpakpermissions 1.0

KCM.ScrollViewKCM {
    id: root
    title: i18n("Flatpak Applications")
    Kirigami.ColumnView.fillWidth: false
    implicitWidth: Kirigami.Units.gridUnit * 40
    implicitHeight: Kirigami.Units.gridUnit * 20
    framedView: false

    function changeApp() {
        let ref = null;
        const row = appsListView.currentIndex;
        if (row !== -1) {
            const m = kcm.refsModel;
            const index = m.index(row, 0);
            ref = m.data(m.index(row, 0), FlatpakReferencesModel.Ref);
        }
        kcm.pop(0)
        kcm.setIndex(row)
        kcm.push("permissions.qml", { ref })
    }

    Component.onCompleted: {
        kcm.columnWidth = Kirigami.Units.gridUnit * 15
        kcm.push("permissions.qml")
    }

    KCM.ConfigModule.buttons: KCM.ConfigModule.Apply | KCM.ConfigModule.Default

    Component {
        id: applyOrDiscardDialogComponent

        Kirigami.PromptDialog {
            id: dialog

            parent: root.Kirigami.ColumnView.view
            title: i18n("Apply Permissions")
            subtitle: i18n("The permissions of this application have been changed. Do you want to apply these changes or discard them?")
            standardButtons: QQC2.Dialog.Apply | QQC2.Dialog.Discard

            onApplied: {
                kcm.save()
                root.changeApp()
                dialog.close()
            }

            onDiscarded: {
                kcm.load()
                root.changeApp()
                dialog.close()
            }

            onRejected: {
                appsListView.currentIndex = kcm.currentIndex()
            }

            onClosed: destroy()
        }
    }

    view: ListView {
        id: appsListView

        model: kcm.refsModel
        currentIndex: -1
        delegate: Kirigami.BasicListItem {

            text: model.name
            icon: model.icon

            function shouldChange() {
                if (index === kcm.currentIndex()) {
                    // Don't reload if it's current anyway.
                    return;
                }
                if (kcm.isSaveNeeded()) {
                    const dialog = applyOrDiscardDialogComponent.createObject(root, {});
                    dialog.open();
                } else {
                    root.changeApp()
                }
            }

            onClicked: shouldChange()
        }
    }
}

