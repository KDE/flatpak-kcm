/*
    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
    SPDX-FileCopyrightText: 2025 David Redondo <kde@david-redondo.de>
*/

import QtQuick
import QtQuick.Controls as QQC
import org.kde.kcmutils as KCMUtils
import org.kde.kitemmodels as KItemModels
import org.kde.kirigami as Kirigami

KCMUtils.ScrollViewKCM
{
    sidebarMode: !Kirigami.Settings.isMobile
    Kirigami.ColumnView.pinned: true

    header: Kirigami.SearchField {
        id: search
        KeyNavigation.tab: view
        KeyNavigation.down: view
    }

    view: ListView {

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
                KCMUtils.ConfigModule.currentIndex = 0;
                KCMUtils.ConfigModule.push("Permissions.qml", {"appId": model.appId, "title": model.display, "isHostApp": model.isHost});
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
}
