/**
 * SPDX-FileCopyrightText: 2022 Suhaas Joshi <joshiesuhaas0@gmail.com>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

import QtQuick 2.12
import QtQuick.Controls 2.12 as Controls
import QtQuick.Layouts 1.15 as Layouts

import org.kde.kirigami 2.19 as Kirigami
import org.kde.kcm 1.2 as KCM
import org.kde.plasma.kcm.flatpakpermissions 1.0

KCM.ScrollViewKCM {
    id: root
    title: i18n("Flatpak Applications")
    Kirigami.ColumnView.fillWidth: false
    implicitWidth: Kirigami.Units.gridUnit * 40
    implicitHeight: Kirigami.Units.gridUnit * 20
    framedView: true

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

            function shouldChange()
            {
                if (kcm.isSaveNeeded()) {
                    promptDialog.open()
                } else {
                    changeApp()
                }
            }

            function changeApp()
            {
                kcm.pop(0)
                kcm.setIndex(appsListView.currentIndex)
                kcm.push("permissions.qml", {ref: model.reference})
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
                    changeApp()
                    promptDialog.close()
                }

                onDiscarded: {
                    kcm.load()
                    changeApp()
                    promptDialog.close()
                }

                onRejected: {
                    appsListView.currentIndex = kcm.currentIndex()
                }
            }
        }
    }
}

