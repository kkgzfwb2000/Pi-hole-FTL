#!/bin/sh
set -e

patch -p1 < patch/civetweb/0001-add-pihole-mods.patch
patch -p1 < patch/civetweb/0001-Add-NO_DLOPEN-option-to-civetweb-s-LUA-routines.patch
patch -p1 < patch/civetweb/0001-Always-Kepler-syntax-for-Lua-server-pages.patch
patch -p1 < patch/civetweb/0001-Add-FTL-URI-rewriting-changes-to-CivetWeb.patch
patch -p1 < patch/civetweb/0001-Add-mbedTLS-debug-logging-hook.patch
patch -p1 < patch/civetweb/0001-Register-CSRF-token-in-conn-request_info.patch
patch -p1 < patch/civetweb/0001-Do-not-try-to-guess-server-hostname-in-Civetweb-when.patch

echo "ALL PATCHES APPLIED OKAY"