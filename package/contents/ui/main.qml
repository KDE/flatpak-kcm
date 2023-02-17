/**
 * SPDX-FileCopyrightText: 2022 Suhaas Joshi <joshiesuhaas0@gmail.com>
 * SPDX-FileCopyrightText: 2023 ivan tkachenko <me@ratijas.tk>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

import QtQuick 2.15
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
                    promptDialog.open()
                } else {
                    root.changeApp()
                }
            }

            onClicked: shouldChange()

            Kirigami.PromptDialog {
                id: promptDialog
                parent: root
                title: i18n("Apply Permissions")
                subtitle: i18n("The permissions of this application have been changed. Do you want to apply these changes or discard them?")
                standardButtons: Kirigami.Dialog.Apply | Kirigami.Dialog.Discard

                onApplied: {
                    kcm.save()
                    root.changeApp()
                    promptDialog.close()
                }

                onDiscarded: {
                    kcm.load()
                    root.changeApp()
                    promptDialog.close()
                }

                onRejected: {
                    appsListView.currentIndex = kcm.currentIndex()
                }
            }
        }
    }
}

