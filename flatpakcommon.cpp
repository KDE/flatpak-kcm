/**
 * Copyright © 2014 Red Hat, Inc
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "flatpakcommon.h"

// Turning off clang-format, because it is very picky and annoying, but we
// don't want to change these lines too much from upstream. It's enough that
// we swapped NULL for nullptr and used different indentation.

/* clang-format off */

/* Same order as enum */
namespace FlatpakStrings
{

const char *flatpak_policy[] = {
    "none",
    "see",
    "talk",
    "own",
    nullptr
};

const char *flatpak_context_shares[] = {
    "network",
    "ipc",
    nullptr
};

const char *flatpak_context_sockets[] = {
    "x11",
    "wayland",
    "pulseaudio",
    "session-bus",
    "system-bus",
    "fallback-x11",
    "ssh-auth",
    "pcsc",
    "cups",
    "gpg-agent",
    nullptr
};

const char *flatpak_context_devices[] = {
    "dri",
    "all",
    "kvm",
    "shm",
    nullptr
};

const char *flatpak_context_features[] = {
    "devel",
    "multiarch",
    "bluetooth",
    "canbus",
    "per-app-dev-shm",
    nullptr
};

const char *flatpak_context_special_filesystems[] = {
    "home",
    "host",
    "host-etc",
    "host-os",
    "host-reset",
    nullptr
};

}

/* clang-format on */
