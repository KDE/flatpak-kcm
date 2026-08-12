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
    required property var decoration

    activeFocusOnTab: true

    actions: [
        Kirigami.Action {
            readonly property var ref: kcm.flatpakRefForApp(root.appId)
            visible: ref
            text: i18nc("@action:intoolbar", "Manage Flatpak Settings")
            icon.name: "flatpak-discover"
            onTriggered: {
                kcm.push("FlatpakPermissions.qml", {"ref": ref, "decoration": root.decoration})
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

    Kirigami.Form {
        id: controlsLayout

        Kirigami.SizeGroup {
            mode: Kirigami.SizeGroup.Width
            items: [screenshotCombobox, cameraCombobox, locationCombobox, wallpaperCombobox]
        }

        Kirigami.FormGroup {
            Kirigami.FormEntry {
                title: i18nc("@label:group", "General:")
                visible: !root.isHostApp
                contentItem: QQC.Switch {
                    id: notificationsSwitch
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
            }

            Kirigami.FormEntry {
                visible: !root.isHostApp
                contentItem: QQC.Switch {
                    id: powerManagementSwitch
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
            }

            Kirigami.FormEntry {
                visible: !root.isHostApp && kcm.gamemodeAvailable
                contentItem: QQC.Switch {
                    id: gameModeSwitch
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
                trailingItems: Kirigami.ContextualHelpButton {
                    id: helper
                    toolTipText: xi18nc("@info:tooltip", "Allows the application to activate game mode if you are using it on your system.")
                }
            }

            Kirigami.FormEntry {
                visible: !root.isHostApp
                contentItem: QQC.Switch {
                    id: highProcessPrioritySwitch
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
            }

            Kirigami.FormSeparator {}

            Kirigami.FormEntry {
                title: i18nc("@label:listbox", "Take screenshots:")
                contentItem: PermissionCombobox {
                    id: screenshotCombobox
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
            }

            Kirigami.FormEntry {
                title: i18nc("@title:group", "Camera access:")
                visible: !root.isHostApp
                contentItem: PermissionCombobox {
                    id: cameraCombobox
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
            }

            Kirigami.FormEntry {
                title: i18nc("@label:listbox", "Location accuracy:")
                visible: !root.isHostApp
                contentItem: PermissionCombobox {
                    id: locationCombobox
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
            }

            Kirigami.FormEntry {
                title: i18nc("@label:listbox", "Set desktop and lock screen background:")
                visible: !root.isHostApp
                contentItem: PermissionCombobox {
                    id: wallpaperCombobox
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
            }


            Kirigami.FormSeparator {}

            Kirigami.FormAction {
                title: i18nc("@label", "Screen sharing:")
                visible: screencastSessions.rowCount > 0
                action: Kirigami.Action {
                    icon.name: "video-display"
                    text: i18ncp("@action:button", "Manage %1 Session", "Manage %1 Sessions", screencastSessions.rowCount)
                    onTriggered: kcm.push("SessionList.qml", {"model": screencastSessions, "title": i18nc("@title:window %1 is the name of the application","%1 – Screencast Sessions", root.title)})
                }
                KCM.ScreencastSessionsModel {
                    id: screencastSessions
                    appId: root.appId
                }
            }

            Kirigami.FormAction {
                title: i18nc("@label", "Capture pointer & keyboard input:")
                visible: inputCaptureSessions.rowCount > 0
                action: Kirigami.Action {
                    icon.name: "dialog-input-devices"
                    text: i18ncp("@action:button", "Manage %1 Session", "Manage %1 Sessions", inputCaptureSessions.rowCount)
                    onTriggered: kcm.push("SessionList.qml", {"model": inputCaptureSessions, "title": i18nc("@title:window %1 is the name of the application","%1 – Input Capture Sessions", root.title)})
                }
                KCM.InputCaptureSessionsModel {
                    id: inputCaptureSessions
                    appId: root.appId
                }
            }

            Kirigami.FormEntry {
                implicitWidth: Kirigami.Units.gridUnit * 20
                title: i18nc("@label 'Remote control' like in xdg-desktop-portal-kde remotedesktopdialog.cpp", " Remote control:")
                contentItem: QQC.Switch {
                    id: remoteControlSwitch
                    Layout.fillWidth: true
                    text: i18nc("@option:check", "Control pointer & keyboard, and share screen with other apps without asking")
                    PermissionItem {
                        id: remoteDesktopKdeAuthorized
                        table: "kde-authorized"
                        resource: "remote-desktop"
                    }
                    checked: remoteDesktopKdeAuthorized.permissions[0] === "yes"
                    onToggled: remoteDesktopKdeAuthorized.permissions = checked ? ["yes"] : ["no"]
                }
            }

            Kirigami.FormAction {
                visible: remoteDesktopSessions.rowCount > 0
                action: Kirigami.Action {
                    icon.name: "krfb"
                    text: i18ncp("@action:button", "Manage %1 Session", "Manage %1 Sessions", remoteDesktopSessions.rowCount)
                    onTriggered: kcm.push("SessionList.qml", {"model": remoteDesktopSessions, "title": i18nc("@title:window %1 is the name of the application", "%1 – Remote Desktop Sessions", root.title)})
                }
                KCM.RemoteDesktopSessionsModel {
                    id: remoteDesktopSessions
                    appId: root.appId
                }
            }
        }
    }
}
