#!/usr/bin/env bash
# Installiert esp-clang (clangd, clang-tidy, clang-format mit Xtensa-Support)
# und legt versionsunabhängige Symlinks in /usr/local/bin an.
set -e

# udevadm wird von der ESP-IDF-Extension zum Auflisten der Ports gebraucht,
# libpython3.12 vom GDB für die libstdc++-Pretty-Printer (std::string etc.)
apt-get update && apt-get install -y --no-install-recommends udev libpython3.12

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
