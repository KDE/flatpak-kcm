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
        displayText: currentIndex === -1 ? i18nc("@item:inlistbox", "Unset") : currentText
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
        PermissionCombobox {
            Kirigami.FormData.label: i18nc("@label:listbox", "Take screenshots:")
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
            visible: !root.isHostApp
            Kirigami.FormData.label: i18nc("@title:group", "Camera Access:")
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
            visible: !root.isHostApp
            Kirigami.FormData.label: i18nc("@label:listbox", "Location accuracy:")
            PermissionItem {
                id: locationPermission
                table: "location"
                resource: "location"
            }
            model: [
                {value: "NONE", text: i18nc("@item:inlistbox location accuracy", "Disallowed")},
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

         QQC.Switch {
            visible: !root.isHostApp
            text: i18nc("@option:check", "Send notifications")
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

        QQC.Switch {
            visible: !root.isHostApp
            text: i18nc("@option:check", "Activate gamemode")
            PermissionItem {
                id: gamemodePermission
                table: "gamemode"
                resource: "gamemode"
            }
            checked: gamemodePermission.permissions[0] !== "no"
            onToggled: gamemodePermission.permissions = checked ? ["yes"] : ["no"]
        }

        QQC.Switch {
            visible: !root.isHostApp
            text: i18nc("@option:check", "Gain higher process priority")
            PermissionItem {
                id: realtimePermission
                table: "realtime"
                resource: "realtime"
            }
            checked: realtimePermission.permissions[0] !== "no"
            onToggled: realtimePermission.permissions = checked ? ["yes"] : ["no"]
        }

        QQC.Button {
            Kirigami.FormData.label: i18nc("@label", "Screen Sharing:")
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
