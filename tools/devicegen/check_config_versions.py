#!/usr/bin/env python3
"""Guard against legacy *ConfigV<n> structs leaking into runtime/adapter code.

Old config-struct versions are immutable persisted formats kept ONLY for decode and
migration (see docs/device-config-versioning.md). Runtime classes, REST/HA adapters and
any other code must use the latest version -- a stray `SomethingConfigV1` in a runtime
means new writes or reads went through an outdated layout.

For each family (the identifier prefix before `ConfigV`) the latest version is the
highest one defined. A reference to any lower version is allowed only in:
  - a file that defines config structs, or its same-stem .h/.cpp sibling (this is where
    the version zoo + migrateFrom/decode legitimately live), or
  - a test file.
Anywhere else under src/ is a violation.

Non-zero exit on any violation, so it can gate scripts/test.sh.
"""
import pathlib
import re
import sys

REPO = pathlib.Path(sys.argv[1] if len(sys.argv) > 1 else ".").resolve()
SRC = REPO / "src"

STRUCT_DEF = re.compile(r"\bstruct\s+([A-Za-z0-9_]+)ConfigV(\d+)\b")
USAGE = re.compile(r"\b([A-Za-z0-9_]+)ConfigV(\d+)\b")

# A legacy version reference on one of these lines is legitimate migration/decode/layout
# plumbing, not active use. Everything else (a member, return type, adapter template
# parameter, encode target, ...) is the thing this guard is meant to catch.
EXEMPT_LINE = re.compile(
    r"migrateFrom"
    r"|\bdecode"                        # decode<X>Config / decodeFixedConfigBlob / decodeValidatedFixedConfigBlob
    r"|readFixedConfigSegment"
    r"|static_assert"
    r"|static_cast<"
    r"|::kMagic"
    r"|const\s+[A-Za-z0-9_]+ConfigV\d+\s*&"  # const-ref parameter/binding (ctor overloads, migrateFrom)
    r"|[A-Za-z0-9_]+ConfigV\d+::"            # version-scoped access, e.g. SwitchDeviceConfigV1::validate()
    r"|ConfigV\d+\s+legacy"                  # decode local named legacy*, e.g. `...ConfigV2 legacyV2{}`
)


def main() -> int:
    files = sorted(SRC.rglob("*.h")) + sorted(SRC.rglob("*.cpp"))
    texts = {f: f.read_text() for f in files}

    # latest version per family, and the files that define any version of a family
    latest: dict[str, int] = {}
    defining_files: set[pathlib.Path] = set()
    for f, txt in texts.items():
        for m in STRUCT_DEF.finditer(txt):
            fam, ver = m.group(1), int(m.group(2))
            latest[fam] = max(latest.get(fam, 0), ver)
            defining_files.add(f)

    # allowed = defining files + their same-stem sibling (.h<->.cpp)
    allowed = set(defining_files)
    for f in defining_files:
        allowed.add(f.with_suffix(".cpp" if f.suffix == ".h" else ".h"))

    problems = []
    for f, txt in texts.items():
        if f in allowed or "/test" in str(f):
            continue
        seen = set()
        for line in txt.splitlines():
            if EXEMPT_LINE.search(line):
                continue
            for m in USAGE.finditer(line):
                fam, ver = m.group(1), int(m.group(2))
                if fam in latest and ver < latest[fam] and (fam, ver) not in seen:
                    seen.add((fam, ver))
                    rel = f.relative_to(REPO)
                    problems.append(f"{rel}: uses legacy {fam}ConfigV{ver} (latest is V{latest[fam]})")

    if problems:
        print(f"[check-config-versions] {len(problems)} legacy config use(s) outside migration code:")
        for p in problems:
            print("  -", p)
        return 1
    print(f"[check-config-versions] OK -- {len(latest)} config families, no legacy version used in runtime/adapter code")
    return 0


if __name__ == "__main__":
    sys.exit(main())
