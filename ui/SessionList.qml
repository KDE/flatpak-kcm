/*
    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
    SPDX-FileCopyrightText: 2025 David Redondo <kde@david-redondo.de>
*/

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC
import org.kde.kcmutils as KCMUtils
import org.kde.kirigami as Kirigami

pragma ComponentBehavior: Bound

KCMUtils.ScrollViewKCM {
    property alias model : view.model
    property alias delegate: view.delegate
    view: Kirigami.CardsListView {
        id: view
        delegate: Kirigami.AbstractCard {
            id: delegate
            required property var model
            required property int index
            text: model.display
            contentItem: ColumnLayout {
                Repeater {
                    model: delegate.model.display
                    delegate: RowLayout {
                        required property var modelData
                        Kirigami.Icon {
                            Layout.alignment: Qt.AlignTop
                            source: parent.modelData.icon
                        }
                        QQC.Label {
                            Layout.fillWidth: true
                            text: parent.modelData.text
                            wrapMode: Text.WordWrap
                        }
                    }
                }
            }
            footer: RowLayout {
                Item {
                    id: spacer
                    Layout.fillWidth: true
                }
                QQC.Button {
                    Layout.fillWidth: Kirigami.Settings.isMobile
                    icon.name: "list-remove-symbolic"
                    text: i18nc("@action:button", "Revoke")
                    onClicked: delegate.ListView.view.model.revoke(delegate.model.id)
                }
            }
        }
    }
}

