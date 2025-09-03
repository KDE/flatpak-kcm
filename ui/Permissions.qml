/*
    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
    SPDX-FileCopyrightText: 2025 David Redondo <kde@david-redondo.de>
*/


import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kcmutils as KCMUtils
import org.kde.plasma.kcm.flatpakpermissions as KCM

pragma ComponentBehavior: Bound

KCMUtils.SimpleKCM {
    id: root
    required property string appId
    required property bool isHostApp

    actions: [
        Kirigami.Action {
            readonly property var ref: kcm.flatpakRefForApp(root.appId)
            visible: ref
            text: i18nc("@action:intoolbar", "Manage Flatpak Settings")
            icon.name: "flatpak-discover"
            onTriggered: {
                kcm.push("FlatpakPermissions.qml", {"ref": ref})
            }
        }
    ]

    component PermissionItem : KCM.PermissionItem {
        appId: root.appId
    }
    component PermissionCombobox : QQC.ComboBox {
        required property string activeValue
        signal valueSelected(value: string)
        textRole: "text"
        valueRole: "value"
        Component.onCompleted: currentIndex = indexOfValue(activeValue)
        onActivated: {
            valueSelected(currentValue)
            currentIndex = Qt.binding(() => indexOfValue(activeValue))
        }
    }

    header: Kirigami.InlineMessage {
        text: i18nc("@info", "Well-behaved applications might respect these settings but even if turned off here they are \
still able to control mouse and keyboard or record the contents of your screen through the windowing system")
        visible: Qt.platform.pluginName === "xcb"
        position: Kirigami.InlineMessage.Header
    }

    Kirigami.FormLayout {

        readonly property double comboboxPreferredWidth: Math.max(screenshotCombobox.implicitWidth, cameraCombobox.implicitWidth, locationCombobox.implicitWidth)

        QQC.Switch {
            Kirigami.FormData.label: i18nc("@label:group", "General:")
            visible: !root.isHostApp
            text: i18nc("@option:check", "Send notifications")
            Layout.fillWidth: true
            PermissionItem {
                id: notificationPermission
                table: "notifications"
                resource: "notification"
            }
            checked: notificationPermission.permissions[0] !== "no"
            onToggled: notificationPermission.permissions = checked ? ["yes"] : ["no"]
        }

        QQC.Switch {
            visible: !root.isHostApp
            Layout.fillWidth: true
            text: i18nc("@option:check", "Block automatic sleep and screen locking")
            PermissionItem {
                id: inhibitPermission
                table: "inhibit"
                resource: "inhibit"
            }
            checked: {
                // If the permission is *unset* the portal allows everything by default
                // Otherwise it's a list of allowed actions "logout", "switch", "suspend" or "idle"
                // Simplified here to a switch
                const perms = inhibitPermission.permissions
                if (perms.length === 0) {
                    return true
                }
                return perms.includes("logout") || perms.includes("switch") || perms.includes("suspend") || perms.includes("idle")
            }
            onToggled: inhibitPermission.permissions = checked ? [] : [""]
        }

        RowLayout {
            visible: !root.isHostApp && kcm.gamemodeAvailable
            QQC.Switch {
                Layout.fillWidth: true
                text: i18nc("@option:check", "Activate game mode")
                PermissionItem {
                    id: gamemodePermission
                    table: "gamemode"
                    resource: "gamemode"
                }
                checked: gamemodePermission.permissions[0] !== "no"
                onToggled: gamemodePermission.permissions = checked ? ["yes"] : ["no"]
            }
            Kirigami.ContextualHelpButton {
                id: helper
                toolTipText: xi18nc("@info:tooltip", "Allows the application to activate game mode if you are using it on your system.")
            }
        }

        QQC.Switch {
            visible: !root.isHostApp
            Layout.fillWidth: true
            text: i18nc("@option:check", "Gain higher process priority")
            PermissionItem {
                id: realtimePermission
                table: "realtime"
                resource: "realtime"
            }
            checked: realtimePermission.permissions[0] !== "no"
            onToggled: realtimePermission.permissions = checked ? ["yes"] : ["no"]
        }

        Item {
            Kirigami.FormData.isSection: true
        }

        PermissionCombobox {
            id: screenshotCombobox
            Kirigami.FormData.label: i18nc("@label:listbox", "Take screenshots:")
            Layout.preferredWidth: parent.comboboxPreferredWidth
            PermissionItem {
                id: screenshotPermission
                table: "screenshot"
                resource: "screenshot"
            }
            model: [
                {value: "", text: i18nc("@item:inlistbox", "Ask Once")},
                {value: "no", text: i18nc("@item:inlistbox", "Deny")},
                {value: "yes", text: i18nc("@item:inlistbox", "Allow")},
                {value: "ask", text: i18nc("@item:inlistbox", "Always Ask")},
            ]
            // Unset/empty makes the portal ask once
            activeValue: screenshotPermission.permissions[0] ?? ""
            onValueSelected: (value) => screenshotPermission.permissions = [value]
        }

        PermissionCombobox {
            id: cameraCombobox
            visible: !root.isHostApp
            Kirigami.FormData.label: i18nc("@title:group", "Camera access:")
            Layout.preferredWidth: parent.comboboxPreferredWidth
            PermissionItem {
                id: cameraPermission
                table: "devices"
                resource: "camera"
            }
            model: [
                {value: "", text: i18nc("@item:inlistbox", "Ask Once")},
                {value: "no", text: i18nc("@item:inlistbox", "Deny")},
                {value: "yes", text: i18nc("@item:inlistbox", "Allow")},
                {value: "ask", text: i18nc("@item:inlistbox", "Always Ask")},
            ]
            // Unset/empty makes the portal ask once
            activeValue: cameraPermission.permissions[0] ?? ""
            onValueSelected: (value) => cameraPermission.permissions = [value]
        }

        PermissionCombobox {
            id: locationCombobox
            visible: !root.isHostApp
            Kirigami.FormData.label: i18nc("@label:listbox", "Location accuracy:")
            Layout.preferredWidth: parent.comboboxPreferredWidth
            PermissionItem {
                id: locationPermission
                table: "location"
                resource: "location"
            }
            displayText: currentIndex === -1 ? i18nc("@item:inlistbox", "Ask Once") : currentText
            model: [
                {value: "NONE", text: i18nc("@item:inlistbox location accuracy", "Deny")},
                {value: "COUNTRY", text: i18nc("@item:inlistbox location accuracy", "Country")},
                {value: "CITY", text: i18nc("@item:inlistbox location accuracy", "City")},
                {value: "NEIGHBORHOOD", text: i18nc("@item:inlistbox location accuracy", "Neighborhood")},
                {value: "STREET", text: i18nc("@item:inlistbox location accuracy", "Street")},
                {value: "EXACT", text: i18nc("@item:inlistbox location accuracy", "Exact")},
            ]
            activeValue: locationPermission.permissions[0] ?? ""
            // The format of the permission is [permission, lastUsageTimestamp], everything else will be rejected
            onValueSelected: {
                if (locationPermission.permissions.length >= 2) {
                    locationPermission.permissions = [value, locationPermission.permissions[1]]
                } else {
                    locationPermission.permissions = [value, 0]
                }
            }
        }

        PermissionCombobox {
            id: wallpaperCombobox
            visible: !root.isHostApp
            Kirigami.FormData.label: i18nc("@label:listbox", "Set desktop and lock screen background:")
            Layout.preferredWidth: parent.comboboxPreferredWidth
            PermissionItem {
                id: wallpaperPermission
                table: "wallpaper"
                resource: "wallpaper"
            }
            model: [
                {value: "", text: i18nc("@item:inlistbox", "Ask Once")},
                {value: "no", text: i18nc("@item:inlistbox", "Deny")},
                {value: "yes", text: i18nc("@item:inlistbox", "Allow")},
                {value: "ask", text: i18nc("@item:inlistbox", "Always Ask")},
            ]
            // Unset/empty makes the portal ask once
            activeValue: wallpaperPermission.permissions[0] ?? ""
            onValueSelected: (value) => wallpaperPermission.permissions = [value]
        }


        Item {
            Kirigami.FormData.isSection: true
        }

        QQC.Button {
            Kirigami.FormData.label: i18nc("@label", "Screen sharing:")
            KCM.ScreencastSessionsModel {
                id: screencastSessions
                appId: root.appId
            }
            text: i18ncp("@action:button", "Manage %1 Session", "Manage %1 Sessions", screencastSessions.rowCount)
            icon.source: "video-display"
            visible: screencastSessions.rowCount > 0
            onClicked: kcm.push("SessionList.qml", {"model": screencastSessions, "title": i18nc("@title:window %1 is the name of the application","%1 – Screencast Sessions", root.title)})
        }

        QQC.Switch {
            Kirigami.FormData.label: i18nc("@label 'Remote control' like in xdg-desktop-portal-kde remotedesktopdialog.cpp", " Remote control:")
            Layout.fillWidth: true
            id: remoteDesktopCheckbox
            text: i18nc("@option:check", "Control pointer & keyboard, and share screen with other apps without asking")
            PermissionItem {
                id: remoteDesktopKdeAuthorized
                table: "kde-authorized"
                resource: "remote-desktop"
            }
            checked: remoteDesktopKdeAuthorized.permissions[0] === "yes"
            onToggled: remoteDesktopKdeAuthorized.permissions = checked ? ["yes"] : ["no"]
        }
        QQC.Button {
            enabled: !remoteDesktopCheckbox.checked
            KCM.RemoteDesktopSessionsModel {
                id: remoteDesktopSessions
                appId: root.appId
            }
            text: i18ncp("@action:button", "Manage %1 Session", "Manage %1 Sessions", remoteDesktopSessions.rowCount)
            icon.source: "krfb"
            visible: remoteDesktopSessions.rowCount > 0
            onClicked: kcm.push("SessionList.qml", {"model": remoteDesktopSessions, "title": i18nc("@title:window %1 is the name of the application", "%1 – Remote Desktop Sessions", root.title)})
        }
    }
}
