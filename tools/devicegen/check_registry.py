#!/usr/bin/env python3
"""Cross-check that every device type is wired into all of its registration points.

Manifest-free: the canonical type set is the REST adapters' `kTypeName`. For each
type it verifies the touchpoints that are easy to forget and fail only at runtime:

  1. REST adapter registered in DeviceApiAdapterRegistry::withDefaults()
  2. TS model exists with a matching TYPE_NAME
  3. TS model class imported into device-ui-registry.ts (UI coverage)
  4. at least one mock seed in database.ts
  5. every localized device catalog lists the type and the current type count

Plus reverse orphans (TS/mock types with no adapter) and duplicate TS TYPE_IDs.

Exit code is non-zero on any problem, so it can gate scripts/test.sh. This is the
"forgot one of ten places" guard from docs/device-scaffolding.md; it does not check
field-level defaults (that needs a per-type manifest -- see generate.py `check`).
"""
import pathlib
import re
import sys

REPO = pathlib.Path(sys.argv[1] if len(sys.argv) > 1 else ".").resolve()


def read(rel: str) -> str:
    p = REPO / rel
    return p.read_text() if p.exists() else ""


def glob_text(pattern: str):
    for p in sorted(REPO.glob(pattern)):
        yield p, p.read_text()


def main() -> int:
    # 1. canonical set: {typeName: adapterClass} from REST adapter headers
    adapters = {}
    for _p, txt in glob_text("src/integrations/rest/**/*.h"):
        cls = re.search(r"class\s+(\w+)\s+final", txt)
        tn = re.search(r'kTypeName\s*=\s*"([^"]+)"', txt)
        if cls and tn:
            adapters[tn.group(1)] = cls.group(1)

    if not adapters:
        print("check-registry: no REST adapters found -- wrong --repo path?")
        return 2

    # 2. adapters actually registered
    reg_txt = read("src/integrations/common/DeviceApiAdapter.cpp")
    registered = set(re.findall(r"registerAdapter\(\s*(\w+)::instance", reg_txt))

    # 3. TS models: {typeName: modelClass}. TYPE_NAME/TYPE_ID belong to the nearest
    # preceding class decl -- models extend various bases (TemperatureSensorDevice,
    # Pcf857xExpanderDeviceBase, ...), not only BaseDevice, so don't gate on the base.
    ts_models = {}
    ts_ids = {}
    for _p, txt in glob_text("portal-spa/src/models/devices/**/*.ts"):
        classes = [(m.start(), m.group(1)) for m in re.finditer(r"\bclass\s+(\w+)", txt)]

        def enclosing(pos):
            cls = None
            for start, name in classes:
                if start <= pos:
                    cls = name
                else:
                    break
            return cls

        for m in re.finditer(r"TYPE_NAME\s*=\s*'([^']+)'", txt):
            cls = enclosing(m.start())
            if cls:
                ts_models[m.group(1)] = cls
        for m in re.finditer(r"TYPE_ID\s*=\s*(\d+)", txt):
            cls = enclosing(m.start())
            tn = next((t for t, c in ts_models.items() if c == cls), None)
            if tn:
                ts_ids.setdefault(int(m.group(1)), []).append(tn)

    ui_txt = read("portal-spa/src/components/devices/registry/device-ui-registry.ts")
    mock_txt = read("portal-spa/src/mock/database.ts")
    mock_types = set(re.findall(r"createDeviceRecord\(\s*[^,]+,\s*'([^']+)'", mock_txt))
    catalog_files = list(REPO.glob("docs-site/src/content/docs/**/reference/devices/index.md"))

    problems = []

    for tn in sorted(adapters):
        adapter_cls = adapters[tn]
        if adapter_cls not in registered:
            problems.append(f"{tn}: adapter {adapter_cls} not in DeviceApiAdapterRegistry::withDefaults()")
        model_cls = ts_models.get(tn)
        if not model_cls:
            problems.append(f"{tn}: no TS model with TYPE_NAME '{tn}'")
        elif model_cls not in ui_txt:
            problems.append(f"{tn}: model {model_cls} not referenced in device-ui-registry.ts")
        if tn not in mock_types:
            problems.append(f"{tn}: no mock seed in database.ts")

    # reverse orphans
    for tn in sorted(set(ts_models) - set(adapters)):
        problems.append(f"{tn}: TS model has no REST adapter (orphan)")
    for tn in sorted(mock_types - set(adapters)):
        problems.append(f"{tn}: mock seed for unknown type (no REST adapter)")

    # duplicate TS TYPE_IDs
    for tid, names in sorted(ts_ids.items()):
        if len(names) > 1:
            problems.append(f"TYPE_ID {tid} shared by: {', '.join(names)}")

    if not catalog_files:
        problems.append("no localized device catalogs found")
    for catalog in catalog_files:
        catalog_text = catalog.read_text()
        relative_path = catalog.relative_to(REPO)
        description = re.search(r"^description:\s*(.+)$", catalog_text, re.MULTILINE)
        declared_count = re.search(r"\b(\d+)\b", description.group(1)) if description else None
        if not declared_count or int(declared_count.group(1)) != len(adapters):
            value = declared_count.group(1) if declared_count else "missing"
            problems.append(f"{relative_path}: device count is {value}, expected {len(adapters)}")
        for tn in sorted(adapters):
            if f"`{tn}`" not in catalog_text:
                problems.append(f"{relative_path}: missing device type '{tn}'")

    if problems:
        print(f"[check-registry] {len(problems)} problem(s) across {len(adapters)} types:")
        for p in problems:
            print("  -", p)
        return 1
    print(f"[check-registry] OK -- {len(adapters)} types consistent across adapter/TS/UI/mock/docs")
    return 0


if __name__ == "__main__":
    sys.exit(main())
