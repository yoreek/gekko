#!/usr/bin/env python3
"""Writes src/generated/Version.h from git state.

Regenerated on every build, not committed to git (see .gitignore). Works both
as a PlatformIO extra_script (`pre:`, run via SCons with an injected `env`)
and as a plain script (`python3 scripts/generate_version_header.py`) -- the
latter is needed because `pio test` does not run platformio.ini's
extra_scripts, unlike `pio run`/`pio check`.
"""
import datetime
import os
import subprocess

try:
    Import("env")  # noqa: F821 - provided by PlatformIO/SCons at exec() time
    ROOT_DIR = env.subst("$PROJECT_DIR")  # noqa: F821
except NameError:
    ROOT_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

OUT_DIR = os.path.join(ROOT_DIR, "src", "generated")
OUT_PATH = os.path.join(OUT_DIR, "Version.h")


def git(*args):
    try:
        return subprocess.check_output(["git", *args], cwd=ROOT_DIR, stderr=subprocess.DEVNULL).decode().strip()
    except Exception:
        return "unknown"


def main():
    version = git("describe", "--tags", "--always")
    build_date = datetime.datetime.now(datetime.timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")

    os.makedirs(OUT_DIR, exist_ok=True)
    with open(OUT_PATH, "w") as f:
        f.write(
            "#pragma once\n\n"
            f'#define EWFM_FIRMWARE_VERSION "{version}"\n'
            f'#define EWFM_FIRMWARE_BUILD_DATE "{build_date}"\n'
        )


if __name__ == "__main__":
    main()
