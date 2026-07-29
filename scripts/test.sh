#!/usr/bin/env sh
set -eu

ROOT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
cd "$ROOT_DIR"

"$ROOT_DIR/scripts/lint.sh"

# Device-type registry consistency: every type wired into REST adapter registration,
# a TS model, the UI registry, and a mock seed. See docs/device-scaffolding.md.
python3 "$ROOT_DIR/tools/devicegen/check_registry.py" "$ROOT_DIR"

# Config versioning: legacy *ConfigV<n> structs must stay in migration/decode code only,
# never as a runtime/adapter's active config. See docs/device-config-versioning.md.
python3 "$ROOT_DIR/tools/devicegen/check_config_versions.py" "$ROOT_DIR"

# Webflash artifacts must derive all partition offsets from the partition table.
python3 -B "$ROOT_DIR/scripts/test_collect_webflash.py"

# `pio test` does not run platformio.ini's extra_scripts (unlike `pio run`), so
# generated/Version.h would be missing on a fresh checkout without this.
python3 "$ROOT_DIR/scripts/generate_version_header.py"

PIO_HOME_DIR="${TMPDIR:-/tmp}/.platformio"
ORIGINAL_PIO_HOME="${HOME}/.platformio"
mkdir -p "$PIO_HOME_DIR"
ln -sfn "$ORIGINAL_PIO_HOME/packages" "$PIO_HOME_DIR/packages"
ln -sfn "$ORIGINAL_PIO_HOME/platforms" "$PIO_HOME_DIR/platforms"

HOME="${TMPDIR:-/tmp}" pio test -e native
