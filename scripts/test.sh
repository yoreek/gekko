#!/usr/bin/env sh
set -eu

ROOT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
cd "$ROOT_DIR"

"$ROOT_DIR/scripts/lint.sh"

# `pio test` does not run platformio.ini's extra_scripts (unlike `pio run`), so
# generated/Version.h would be missing on a fresh checkout without this.
python3 "$ROOT_DIR/scripts/generate_version_header.py"

PIO_HOME_DIR="${TMPDIR:-/tmp}/.platformio"
ORIGINAL_PIO_HOME="${HOME}/.platformio"
mkdir -p "$PIO_HOME_DIR"
ln -sfn "$ORIGINAL_PIO_HOME/packages" "$PIO_HOME_DIR/packages"
ln -sfn "$ORIGINAL_PIO_HOME/platforms" "$PIO_HOME_DIR/platforms"

HOME="${TMPDIR:-/tmp}" pio test -e native
