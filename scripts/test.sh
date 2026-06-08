#!/usr/bin/env sh
set -eu

ROOT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
cd "$ROOT_DIR"

"$ROOT_DIR/scripts/lint.sh"

PIO_HOME_DIR="${TMPDIR:-/tmp}/.platformio"
ORIGINAL_PIO_HOME="${HOME}/.platformio"
mkdir -p "$PIO_HOME_DIR"
ln -sfn "$ORIGINAL_PIO_HOME/packages" "$PIO_HOME_DIR/packages"
ln -sfn "$ORIGINAL_PIO_HOME/platforms" "$PIO_HOME_DIR/platforms"

HOME="${TMPDIR:-/tmp}" pio test -e native
