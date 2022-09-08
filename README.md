<!---
SPDX-FileCopyrightText: 2022 Suhaas Joshi <joshiesuhaas0@gmail.com>
SPDX-License-Identifier: GPL-2.0-or-later
-->

# Flatpak Permissions Management KCM

Still under development.

![alt text](https://invent.kde.org/jsuhaas/flatpak-kcm/uploads/cd0eee337bc3568af2f2d74a95a8e413/TITLES.png)

## Building and Running:

- cd wherever/you/like
- git clone [ the link you copy from the clone option ]
- cd flatpak_kcm
- mkdir build
- cd build
- cmake .. -DCMAKE_INSTALL_PREFIX=~/.local/kde
- make install
- source prefix.sh
- kcmshell5 kcm_flatpak
