/*
    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
    SPDX-FileCopyrightText: 2025 David Redondo <kde@david-redondo.de>
*/

import QtQuick
import QtQuick.Controls as QQC
import org.kde.kcmutils as KCMUtils
import org.kde.kitemmodels as KItemModels
import org.kde.kirigami as Kirigami

pragma ComponentBehavior: Bound

KCMUtils.ScrollViewKCM
{
    id: root

    sidebarMode: !Kirigami.Settings.isMobile
    Kirigami.ColumnView.pinned: true

    property int currentIndex: 0;
    KCMUtils.ConfigModule.onCurrentIndexChanged: (index) => {
        // Clicking on the back arrow while there are still unsaved changes
        if (KCMUtils.ConfigModule.needsSave && index != currentIndex) {
            KCMUtils.ConfigModule.currentIndex = currentIndex;
            conflictDialogLoader.active = true
            conflictDialogLoader.item.closed.connect(() => KCMUtils.ConfigModule.currentIndex = index)
        } else {
            currentIndex = index
        }
    }

    header: Kirigami.SearchField {
        id: search
        KeyNavigation.tab: view
        KeyNavigation.down: view
    }

    view: ListView {
        id: appsView

        Accessible.role: Accessible.List

        currentIndex: -1
        model: KItemModels.KSortFilterProxyModel {
            sourceModel: kcm.appsModel
            sortRole: Qt.DisplayRole
            sortCaseSensitivity: Qt.CaseInsensitive
            sortColumn: 0
            filterString: search.text
            filterRoleName: "display"
            filterCaseSensitivity: Qt.CaseInsensitive
        }
        delegate: QQC.ItemDelegate {
            required property var model
            required property int index
            width: view.width
            text: model.display
            icon.source: model.decoration
            highlighted: view.currentIndex == index
            onClicked: {
                if (KCMUtils.ConfigModule.needsSave) {
                    conflictDialogLoader.active = true
                    conflictDialogLoader.item.closed.connect(clicked)
                    return;
                }
                KCMUtils.ConfigModule.currentIndex = 0
                KCMUtils.ConfigModule.push("Permissions.qml", {"appId": model.appId, "title": model.display, "isHostApp": model.isHost, "decoration": icon.source });
                view.currentIndex = index;
            }
            Keys.onEnterPressed: click()
            Keys.onReturnPressed: click()
        }
    }

    KCMUtils.SimpleKCM {
        id: placeholder
        Kirigami.PlaceholderMessage {
            text: i18nc("@info:placeholder", "Select an application from the list to view its permissions here")
            anchors.fill: parent
            anchors.margins: Kirigami.Units.largeSpacing * 2
        }
    }
    Component.onCompleted: KCMUtils.ConfigModule.push(placeholder)

Loader {
        id: conflictDialogLoader
        active: false
        sourceComponent: ApplyOrDiscardDialog {
            visible: true
            applicationName: appsView.currentItem.text
            applicationIcon: appsView.currentItem.icon.source
            onApplied: {
                kcm.save()
                close()
            }
            onDiscarded: {
                kcm.load()
                close()
            }
            onRejected: {
                kcm.load()
            }
            onClosed: {
                conflictDialogLoader.active = false;
            }
        }
    }
}
