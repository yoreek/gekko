#!/usr/bin/env sh
set -eu

ROOT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
cd "$ROOT_DIR"

FORMAT_FILES="$(find src test -type f \( -name '*.h' -o -name '*.hpp' -o -name '*.cpp' -o -name '*.c' \) | sort)"

if ! command -v clang-format >/dev/null 2>&1; then
    echo "error: clang-format is not installed or not available in PATH" >&2
    echo "install it first, for example: sudo apt-get install clang-format" >&2
    exit 127
fi

if ! command -v cppcheck >/dev/null 2>&1; then
    echo "error: cppcheck is not installed or not available in PATH" >&2
    echo "install it first, for example: sudo apt-get install cppcheck" >&2
    exit 127
fi

echo "==> clang-format"
# shellcheck disable=SC2086
clang-format --dry-run --Werror $FORMAT_FILES

echo "==> cppcheck"
cppcheck \
    --enable=warning,performance,portability \
    --error-exitcode=1 \
    --inline-suppr \
    --language=c++ \
    --std=c++17 \
    --suppress=missingIncludeSystem \
    --template=gcc \
    -DUNIT_TEST \
    -Isrc \
    src test

echo "checks passed"
