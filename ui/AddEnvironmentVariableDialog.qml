/**
 * SPDX-FileCopyrightText: 2023 ivan tkachenko <me@ratijas.tk>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

import QtQuick 2.15
import QtQuick.Controls 2.15 as QQC2
import QtQuick.Layouts 1.15

import org.kde.kirigami 2.20 as Kirigami
import org.kde.plasma.kcm.flatpakpermissions 1.0

Kirigami.PromptDialog {
    id: root

    required property FlatpakPermissionModel model

    title: i18n("Set Environment Variable")
    standardButtons: QQC2.Dialog.Ok | QQC2.Dialog.Discard
    closePolicy: QQC2.Popup.CloseOnEscape

    QQC2.Overlay.modal: KcmPopupModal {}

    Kirigami.FormLayout {
        QQC2.TextField {
            id: nameField

            Layout.fillWidth: true

            Keys.onEnterPressed: valueField.forceActiveFocus(Qt.TabFocusReason)
            Keys.onReturnPressed: valueField.forceActiveFocus(Qt.TabFocusReason)
            KeyNavigation.down: valueField

            Kirigami.FormData.label: i18nc("@label:textbox name of environment variable", "Name:")
        }

        QQC2.TextField {
            id: valueField

            Layout.fillWidth: true

            Keys.onEnterPressed: root.accepted()
            Keys.onReturnPressed: root.accepted()

            Kirigami.FormData.label: i18nc("@label:textbox value of environment variable", "Value:")
            // No validation needed, empty value is also acceptable.
        }
    }

    function acceptableInput() {
        const name = nameField.text;

        if (permsModel.permissionExists(FlatpakPermissionsSectionType.Environment, name)) {
            return false;
        }

        return permsModel.isEnvironmentVariableNameValid(name);
    }

    onOpened: {
        const ok = standardButton(QQC2.Dialog.Ok);
        ok.enabled = Qt.binding(() => acceptableInput());
        ok.KeyNavigation.up = valueField;

        const discard = standardButton(QQC2.Dialog.Discard);
        discard.KeyNavigation.up = valueField;

        nameField.forceActiveFocus(Qt.PopupFocusReason);
    }

    onAccepted: {
        if (acceptableInput()) {
            const name = nameField.text;
            const value = valueField.text;
            model.addUserEnteredPermission(FlatpakPermissionsSectionType.Environment, name, value);
            close();
        }
    }

    onDiscarded: {
        close();
    }
}
