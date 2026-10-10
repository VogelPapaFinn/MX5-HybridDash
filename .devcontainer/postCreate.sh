#!/usr/bin/env bash
# Installiert esp-clang (clangd, clang-tidy, clang-format mit Xtensa-Support)
# und legt versionsunabhängige Symlinks in /usr/local/bin an.
set -e

. "$IDF_PATH/export.sh"
idf_tools.py install esp-clang

CLANGD="$(find /opt/esp/tools/esp-clang -type f -path '*/bin/clangd' | sort | tail -n 1)"
if [ -z "$CLANGD" ]; then
    echo "esp-clang clangd nicht gefunden" >&2
    exit 1
fi
CLANG_BIN="$(dirname "$CLANGD")"

for TOOL in clangd clang-tidy clang-format; do
    ln -sf "$CLANG_BIN/$TOOL" "/usr/local/bin/$TOOL"
done

clangd --version
