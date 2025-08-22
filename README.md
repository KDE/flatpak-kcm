<!--
    SPDX-License-Identifier: CC-BY-SA-4.0
    SPDX-FileCopyrightText: 2025 David Redondo <kde@david-redondo.de>
-->

# What is this

A KCM to configure permissions for portal interactions. It also allows changing flatpak settings
via the subsumed flatpak kcm

Note: Some permissions dont make sense to show for non
sandboxed apps as they are only proxying dbus.

## `xdg-desktop-portal` frontend permissions:

These control mostly if the frontend will prompt via the access portal
or will ask the backend to prompt.
|                                                   | show for non-sandboxed |   |
| ------------------------------------------------- | ---------------------- | -- |
| location                                          | 🚫                     | ✔️ |
| notification (to notification kcm?)               | 🚫                     | ✔  |
| gamemode                                          | 🚫                     | ✔️ |
| realtime                                          | 🚫                     | ✔️ |
| screenshot                                        | ✔️                     | ✔️ |
| wallpaper (once implemented)                      | ❔                     | ☐  |
| camera                                            | 🚫                     | ✔️ |
| inhibit (broken impl)                             | 🚫                     | ✔️ |
| usb (we don't have the portal yet)                | 🚫                     | ☐  |
| background - maybe, we allow always at the moment | 🚫                     | ☐  |


OpenURI also uses the permissions store but saves app specific mime choices there, see
https://bugs.kde.org/show_bug.cgi?id=499787

## Our own permission system
Everything that is not above but prompts for permission and doesn't requite the user to make a choice
- Remote Desktop ✔️
- ???
## Restorable sessions
User should be able to revoke granted sessions with "Allow remember".
- Screencast  ✔️
- Remote Desktop  ✔️
