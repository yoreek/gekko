#!/usr/bin/env python3
"""PlatformIO pre-build script: writes src/generated/Version.h from git state.

Regenerated on every build, not committed to git (see .gitignore).
"""
import datetime
import os
import subprocess

Import("env")  # noqa: F821 - provided by PlatformIO/SCons at exec() time

ROOT_DIR = env.subst("$PROJECT_DIR")
OUT_DIR = os.path.join(ROOT_DIR, "src", "generated")
OUT_PATH = os.path.join(OUT_DIR, "Version.h")


def git(*args):
    try:
        return subprocess.check_output(["git", *args], cwd=ROOT_DIR, stderr=subprocess.DEVNULL).decode().strip()
    except Exception:
        return "unknown"


def main():
    version = git("describe", "--tags", "--always", "--dirty")
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
