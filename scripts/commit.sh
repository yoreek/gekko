#!/usr/bin/env sh
set -eu

ROOT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
LOCK_DIR="$ROOT_DIR/.git/gekko-commit"
LOG_FILE="${TMPDIR:-/tmp}/gekko-commit.log"

usage() {
    echo "usage: scripts/commit.sh \"commit message\""
    echo "       scripts/commit.sh --status"
}

status() {
    if [ ! -d "$LOCK_DIR" ]; then
        echo "No wrapper commit is running."
        return
    fi

    commit_pid="$(sed -n '1p' "$LOCK_DIR/pid" 2>/dev/null || true)"
    if [ -n "$commit_pid" ] && kill -0 "$commit_pid" 2>/dev/null; then
        echo "Commit is running (PID $commit_pid). Log: $LOG_FILE"
    else
        echo "Commit lock is stale. Log: $LOG_FILE"
    fi
    tail -n 20 "$LOG_FILE" 2>/dev/null || true
}

case "${1:-}" in
    --help|-h)
        usage
        exit 0
        ;;
    --status)
        if [ "$#" -ne 1 ]; then
            usage >&2
            exit 64
        fi
        status
        exit 0
        ;;
esac

if [ "$#" -ne 1 ] || [ -z "$1" ]; then
    usage >&2
    exit 64
fi

if ! mkdir "$LOCK_DIR" 2>/dev/null; then
    locked_pid="$(sed -n '1p' "$LOCK_DIR/pid" 2>/dev/null || true)"
    if [ -n "$locked_pid" ] && kill -0 "$locked_pid" 2>/dev/null; then
        status >&2
        exit 75
    fi
    rm -f "$LOCK_DIR/pid"
    rmdir "$LOCK_DIR" 2>/dev/null || {
        status >&2
        exit 75
    }
    mkdir "$LOCK_DIR"
fi

commit_pid=""
cleanup() {
    if [ -n "$commit_pid" ] && kill -0 "$commit_pid" 2>/dev/null; then
        kill "$commit_pid" 2>/dev/null || true
        wait "$commit_pid" 2>/dev/null || true
    fi
    rm -f "$LOCK_DIR/pid"
    rmdir "$LOCK_DIR" 2>/dev/null || true
}
trap cleanup EXIT
trap 'exit 129' HUP
trap 'exit 130' INT
trap 'exit 143' TERM

cd "$ROOT_DIR"
: >"$LOG_FILE"

git commit -m "$1" >"$LOG_FILE" 2>&1 &
commit_pid=$!
echo "$commit_pid" >"$LOCK_DIR/pid"
echo "Commit started (PID $commit_pid). Log: $LOG_FILE"

last_stage=""
while kill -0 "$commit_pid" 2>/dev/null; do
    current_stage="$(sed -n '/^==>/p' "$LOG_FILE" | tail -n 1)"
    if [ -n "$current_stage" ] && [ "$current_stage" != "$last_stage" ]; then
        echo "$current_stage"
        last_stage="$current_stage"
    fi
    sleep 5
done

set +e
wait "$commit_pid"
commit_status=$?
set -e
commit_pid=""

echo "Commit finished with exit code $commit_status. Last log lines:"
tail -n 80 "$LOG_FILE"
exit "$commit_status"
