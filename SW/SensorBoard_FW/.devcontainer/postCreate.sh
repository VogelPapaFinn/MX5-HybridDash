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

# ---------------------------------------------------------------------------
# EIM-Setup für die ESP-IDF-Extension (>= 2.x) registrieren.
# Das Image ist ohne EIM installiert, daher fehlt eim_idf.json und
# "Open ESP-IDF Terminal" findet kein Aktivierungsskript.
# Die Extension ruft das Skript mit "-e" auf (gibt KEY=VALUE aus) und
# sourct es ohne Argumente im Terminal.
# ---------------------------------------------------------------------------
ACTIVATE_SCRIPT="/opt/esp/activate_idf_v${ESP_IDF_VERSION}.sh"
cat > "$ACTIVATE_SCRIPT" <<'ACT'
#!/usr/bin/env bash
export IDF_PATH="${IDF_PATH:-/opt/esp/idf}"
export IDF_TOOLS_PATH="${IDF_TOOLS_PATH:-/opt/esp}"
if [ "$1" = "-e" ]; then
    . "$IDF_PATH/export.sh" > /dev/null 2>&1
    for VAR in PATH IDF_PATH IDF_TOOLS_PATH IDF_PYTHON_ENV_PATH ESP_IDF_VERSION \
               OPENOCD_SCRIPTS ESP_ROM_ELF_DIR IDF_CCACHE_ENABLE; do
        [ -n "${!VAR}" ] && echo "$VAR=${!VAR}"
    done
    exit 0
fi
. "$IDF_PATH/export.sh" > /dev/null
ACT
chmod +x "$ACTIVATE_SCRIPT"

mkdir -p "$HOME/.espressif/tools"
cat > "$HOME/.espressif/tools/eim_idf.json" <<JSON
{
  "version": "1.0",
  "gitPath": "$(command -v git)",
  "idfSelectedId": "esp-idf-docker",
  "idfInstalled": [
    {
      "id": "esp-idf-docker",
      "name": "ESP-IDF v${ESP_IDF_VERSION} (Docker)",
      "path": "$IDF_PATH",
      "idfToolsPath": "$IDF_TOOLS_PATH",
      "python": "$IDF_PYTHON_ENV_PATH/bin/python",
      "activationScript": "$ACTIVATE_SCRIPT"
    }
  ]
}
JSON
