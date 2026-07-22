#!/usr/bin/env python3
"""Пилот генератора устройств из манифеста (gpio_switch).

Режимы:
  generate  -- печатает field-слой (C++ JSON-методы, TS-контракт/дефолты/normalize,
               строки реестров) из манифеста.
  check     -- сверяет манифест с РЕАЛЬНЫМ кодом в репозитории и ищет расхождения
               (typeName/typeId по местам регистрации, дефолт поля C++ vs TS).

Пилот намеренно узкий: показывает механизм на живом типе, а не претендует на
полный codegen. Payoff растёт пропорционально числу собственных полей типа.
"""
import argparse
import pathlib
import re
import sys

import yaml

# Маппинг типов манифеста -> C++ / TS / ArduinoJson-аксессор.
CPP_T = {"u8": "uint8_t", "u16": "uint16_t", "i32": "int32_t", "bool": "bool", "float": "float"}
TS_T = {"u8": "number", "u16": "number", "i32": "number", "bool": "boolean", "float": "number"}


def cpp_camel(type_name: str) -> str:
    return "".join(p.capitalize() for p in type_name.split("_"))


def json_read_cpp(f) -> str:
    """Строка чтения одного поля из JSON в C++ (как в существующем parseJson)."""
    name = f["name"]
    if f["type"] == "bool":
        return f'    {name} = input["{name}"] | {name};'
    # целочисленные: приведение через int, как в рукописном коде
    return f'    {name} = static_cast<{CPP_T[f["type"]]}>(input["{name}"] | static_cast<int>({name}));'


def json_read_ts(f) -> str:
    name = f["name"]
    if f["type"] == "bool":
        return f"    {name}: typeof raw.{name} === 'boolean' ? raw.{name} : defaults.{name},"
    return (f"    {name}: typeof raw.{name} === 'number' && Number.isFinite(raw.{name}) "
            f"? raw.{name} : defaults.{name},")


def emit(m: dict) -> str:
    cls = cpp_camel(m["typeName"]) + "Device"
    cfg = f"{cpp_camel(m['typeName'])}DeviceConfigV{m['configVersion']}"
    base_cpp = "SwitchDeviceConfigV2" if m.get("extends") == "switch" else "DeviceBaseConfigV1"
    base_ts = "BaseDeviceConfig, SwitchConfigDraft" if m.get("extends") == "switch" else "BaseDeviceConfig"
    fields = m["fields"]
    out = []

    out.append(f"// ===== GENERATED field-surface for {m['typeName']} — DO NOT EDIT =====\n")

    # --- C++ parseJson ---
    out.append(f"// {cfg}.cpp")
    out.append(f"bool {cfg}::parseJson(const JsonObjectConst& input, const char*& error) {{")
    out.append(f"    if (!{base_cpp}::parseJson(input, error)) return false;")
    for f in fields:
        out.append(json_read_cpp(f))
        if f.get("validate") == "custom":
            hook = f"{m['typeName']}ConfigValidateField_{f['name']}"
            out.append(f"    if (!{hook}(*this, error)) return false;  // <- HANDWRITTEN hook")
    out.append("    return true;")
    out.append("}\n")

    # --- C++ writeJson ---
    out.append(f"void {cfg}::writeJson(JsonObject output) const {{")
    out.append(f"    {base_cpp}::writeJson(output);")
    for f in fields:
        out.append(f'    output["{f["name"]}"] = {f["name"]};')
    out.append("}\n")

    # --- TS contract + defaults + normalize (own fields) ---
    draft = f"{cpp_camel(m['typeName'])}ConfigDraft"
    out.append(f"// {m['typeName']}.ts")
    out.append(f"export interface {draft} extends {base_ts} {{")
    for f in fields:
        out.append(f"  {f['name']}: {TS_T[f['type']]}")
    out.append("}\n")
    out.append("// defaultConfig() own fields:")
    for f in fields:
        v = "true" if f["default"] is True else "false" if f["default"] is False else f["default"]
        out.append(f"//   {f['name']}: {v},")
    out.append("// normalizeConfig() own fields:")
    for f in fields:
        out.append("//" + json_read_ts(f))

    # --- реестры ---
    const = re.sub(r"(?<!^)(?=[A-Z])", "_", cpp_camel(m["typeName"])).upper() + "_DEVICE_TYPE_ID"
    out.append("\n// registration lines:")
    out.append(f"//   device-type-ids.ts : export const {const} = {cls}.TYPE_ID  // {m['typeId']}")
    out.append(f"//   ui-registry.ts     : {{ typeId, typeName: '{m['typeName']}', labelKey: '{m['label']}', "
               f"icon: '{m['icon']}', fields: {m['ui']['fields']}, widget: {m['ui']['widget']} }}")
    out.append(f"//   descriptor         : providedRoles = {m['roles']['provides']}")
    return "\n".join(out)


# ---------- check ----------
ROOT = pathlib.Path(__file__).resolve()
# в пилоте корень репозитория задаётся флагом --repo


def _read(repo: pathlib.Path, rel: str) -> str:
    p = repo / rel
    return p.read_text() if p.exists() else ""


def check(m: dict, repo: pathlib.Path) -> int:
    problems = []
    tn = m["typeName"]

    adapter = _read(repo, f"src/integrations/rest/{tn}/{cpp_camel(tn)}DeviceApiAdapter.h")
    ts_model = _read(repo, f"portal-spa/src/models/devices/{tn.replace('_', '-')}.ts")
    cpp_hdr = _read(repo, f"src/devices/switch/{tn.split('_')[0]}/{cpp_camel(tn)}Device.h")

    # 1) typeName согласован в адаптере и TS-модели
    if f'kTypeName = "{tn}"' not in adapter:
        problems.append(f"typeName '{tn}' не найден в REST-адаптере")
    if f"TYPE_NAME = '{tn}'" not in ts_model:
        problems.append(f"typeName '{tn}' не найден в TS-модели")

    # 2) typeId в TS совпадает с манифестом
    mid = re.search(r"TYPE_ID\s*=\s*(\d+)", ts_model)
    if mid and int(mid.group(1)) != m["typeId"]:
        problems.append(f"typeId: манифест={m['typeId']}, TS={mid.group(1)}")

    # 3) дефолт каждого поля: C++ struct vs TS defaultConfig vs манифест
    for f in m["fields"]:
        name = f["type"] and f["name"]
        cpp_def = re.search(rf"{f['name']}\{{(\w+)\}}", cpp_hdr)
        # только литерал дефолта (число/bool), не объявление типа `: number`
        ts_def = re.search(rf"{f['name']}:\s*(\d+|true|false)\b", ts_model)
        c = cpp_def.group(1) if cpp_def else "?"
        t = ts_def.group(1) if ts_def else "?"
        man = str(f["default"]).lower()
        if c != man or t != man:
            problems.append(
                f"дефолт '{f['name']}': манифест={man}  C++struct={c}  TSdefault={t}  <-- РАСХОЖДЕНИЕ")

    if problems:
        print(f"[check {tn}] найдено {len(problems)} расхождений:")
        for p in problems:
            print("  -", p)
        return 1
    print(f"[check {tn}] OK — манифест и код согласованы")
    return 0


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("manifest")
    ap.add_argument("mode", choices=["generate", "check"])
    ap.add_argument("--repo", default=".")
    a = ap.parse_args()
    m = yaml.safe_load(pathlib.Path(a.manifest).read_text())
    if a.mode == "generate":
        print(emit(m))
        return 0
    return check(m, pathlib.Path(a.repo).resolve())


if __name__ == "__main__":
    sys.exit(main())
