#!/usr/bin/env sh
set -eu

ROOT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
cd "$ROOT_DIR"

if ! command -v clang-format >/dev/null 2>&1; then
    echo "error: clang-format is not installed or not available in PATH" >&2
    exit 127
fi

if ! command -v pio >/dev/null 2>&1; then
    echo "error: pio is not installed or not available in PATH" >&2
    exit 127
fi

PIO_HOME_DIR="${TMPDIR:-/tmp}/.platformio"
ORIGINAL_PIO_HOME="${HOME}/.platformio"
mkdir -p "$PIO_HOME_DIR"
ln -sfn "$ORIGINAL_PIO_HOME/packages" "$PIO_HOME_DIR/packages"
ln -sfn "$ORIGINAL_PIO_HOME/platforms" "$PIO_HOME_DIR/platforms"

SOURCE_FILES="$(find src test -type f \( -name '*.h' -o -name '*.hpp' -o -name '*.cpp' -o -name '*.c' \) | sort)"

echo "==> clang-format"
# shellcheck disable=SC2086
clang-format --dry-run --Werror $SOURCE_FILES

echo "==> pio check (esp32dev)"
env HOME="${TMPDIR:-/tmp}" pio check -e esp32dev --skip-packages --fail-on-defect low --src-filters '+<src/>'

echo "==> pio check (native)"
env HOME="${TMPDIR:-/tmp}" pio check -e native --skip-packages --fail-on-defect low --src-filters '+<src/>' --src-filters '+<test/>'
