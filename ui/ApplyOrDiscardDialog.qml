/**
 * SPDX-FileCopyrightText: 2022 Suhaas Joshi <joshiesuhaas0@gmail.com>
 * SPDX-FileCopyrightText: 2023 ivan tkachenko <me@ratijas.tk>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami

Kirigami.PromptDialog {
    id: dialog

    required property string applicationName
    required property url applicationIcon

    readonly property bool narrow: (parent.width - leftMargin - rightMargin) < Kirigami.Units.gridUnit * 20

    parent: root.Kirigami.ColumnView.view
    title: i18nc("@title:window", "Apply Permissions")
    subtitle: i18nc("@info dialog main text", "The permissions of application %1 have been changed. Do you want to apply these changes or discard them?", applicationName)
    standardButtons: QQC2.Dialog.Apply | QQC2.Dialog.Discard

    GridLayout {
        columns: dialog.narrow ? 1 : 2
        columnSpacing: Kirigami.Units.largeSpacing
        rowSpacing: Kirigami.Units.largeSpacing

        Kirigami.Icon {
            source: dialog.applicationIcon.toString() !== "" ? dialog.applicationIcon : "application-vnd.flatpak.ref"

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
}
