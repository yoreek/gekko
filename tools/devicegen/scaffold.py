#!/usr/bin/env python3
"""Scaffold a new device type from a manifest: stamp the touchpoint files and print the
registration lines to paste into the five registries.

It removes the "create ~10 files + remember 5 registration lines" toil (see the checklist
in docs/device-scaffolding.md). It does NOT write fully-compiling family logic -- the C++
config parse/validate and the runtime hooks are stubbed with `// TODO` markers. The
mechanical, error-prone parts (frontend model, Vue stubs, and the registry snippets) are
generated in full. Run check_registry.py afterwards to confirm nothing was missed.

Usage:
    python3 tools/devicegen/scaffold.py <manifest.device.yaml> --out <dir>

Manifest (superset of the generate.py pilot format):
    typeName: co2_sensor
    typeId: 42
    configVersion: 1
    label: device.type.co2Sensor
    icon: gauge
    className: Co2Sensor          # optional; defaults to CamelCase(typeName)
    cppBase: DeviceRuntimeBase     # runtime base class to extend
    roles: { provides: [], requires: [] }
    fields:
      - { name: pin, kind: pin, default: 4 }
      - { name: warmupMs, kind: uint32, default: 30000 }
"""
import argparse
import pathlib
import re
import sys

import yaml

CPP_T = {"uint8": "uint8_t", "pin": "uint8_t", "uint16": "uint16_t", "uint32": "uint32_t",
         "int32": "int32_t", "float": "float", "bool": "bool"}
TS_T = {"uint8": "number", "pin": "number", "uint16": "number", "uint32": "number",
        "int32": "number", "float": "number", "bool": "boolean"}


def camel(name: str) -> str:
    return "".join(p.capitalize() for p in name.split("_"))


def screaming(name: str) -> str:
    return name.upper()


def cpp_read(f) -> str:
    n = f["name"]
    if f["kind"] == "bool":
        return f'    {n} = input["{n}"] | {n};'
    if f["kind"] == "float":
        return f'    {n} = input["{n}"] | {n};'
    return f'    {n} = static_cast<{CPP_T[f["kind"]]}>(input["{n}"] | static_cast<int>({n}));'


def ts_default(f):
    v = f.get("default", 0)
    return "true" if v is True else "false" if v is False else v


def ts_norm(f) -> str:
    n = f["name"]
    if f["kind"] == "bool":
        return f"      {n}: typeof raw.{n} === 'boolean' ? raw.{n} : defaults.{n},"
    return (f"      {n}: typeof raw.{n} === 'number' && Number.isFinite(raw.{n}) "
            f"? raw.{n} : defaults.{n},")


def render(m: dict) -> dict:
    tn = m["typeName"]
    cls = m.get("className", camel(tn))
    cfg = f"{cls}DeviceConfigV{m.get('configVersion', 1)}"
    base = m.get("cppBase", "DeviceRuntimeBase")
    magic = (cls[:6].upper() + "1")[:8]
    fields = m.get("fields", [])
    files = {}

    # ---- C++ config header ----
    field_decls = "\n".join(f"    {CPP_T[f['kind']]} {f['name']}{{{int(f.get('default', 0)) if f['kind'] != 'bool' else str(f.get('default', False)).lower()}}};"
                            for f in fields)
    files[f"src/devices/{tn}/{cls}DeviceConfig.h"] = f'''#pragma once

#include "devices/core/DeviceBaseConfig.h"

#include <ArduinoJson.h>
#include <cstddef>

namespace ewfm {{

#pragma pack(push, 1)
struct {cfg} : DeviceBaseConfigV1 {{
    static constexpr char kMagic[] = "{magic}";
{field_decls}

    bool parseJson(const JsonObjectConst& input, const char*& error);
    DeviceValidationResult validate() const;
    void writeJson(JsonObject output) const;
}};
#pragma pack(pop)

constexpr size_t {tn}ConfigSize(const {cfg}&) {{
    return sizeof({cfg}::kMagic) - 1U + sizeof({cfg});
}}

}} // namespace ewfm
'''

    # ---- C++ config cpp ----
    reads = "\n".join(cpp_read(f) for f in fields)
    writes = "\n".join(f'    output["{f["name"]}"] = {f["name"]};' for f in fields)
    files[f"src/devices/{tn}/{cls}DeviceConfig.cpp"] = f'''#include "devices/{tn}/{cls}DeviceConfig.h"

namespace ewfm {{

bool {cfg}::parseJson(const JsonObjectConst& input, const char*& error) {{
    if (!DeviceBaseConfigV1::parseJson(input, error)) {{
        return false;
    }}
{reads}
    // TODO: validate ranges and set `error` on failure.
    return true;
}}

DeviceValidationResult {cfg}::validate() const {{
    const DeviceValidationResult base = DeviceBaseConfigV1::validate();
    if (!base.ok()) {{
        return base;
    }}
    // TODO: field validation.
    return {{}};
}}

void {cfg}::writeJson(JsonObject output) const {{
    DeviceBaseConfigV1::writeJson(output);
{writes}
}}

}} // namespace ewfm
'''

    # ---- C++ runtime header (skeleton) ----
    files[f"src/devices/{tn}/{cls}Device.h"] = f'''#pragma once

#include "devices/core/{base}.h"
#include "devices/{tn}/{cls}DeviceConfig.h"

namespace ewfm {{

constexpr DeviceTypeId k{cls}TypeId = {m["typeId"]};

class {cls}Device final : public {base} {{
public:
    {cls}Device(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob);

    const {cfg}& config() const;
    bool serializeConfigBlob(DeviceConfigBlob& configBlob) const override;
    bool applyConfig(const DeviceConfigBlob& configBlob, uint32_t now) override;

    static DeviceTypeDescriptor descriptor();
    static std::unique_ptr<IDeviceRuntime> createRuntime(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob);
    static DeviceValidationResult validateConfig(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob);

private:
    const DeviceBaseConfigV1& baseConfig() const override;
    // TODO: state machine states + hardware hooks.

    {cfg} config_{{}};
}};

}} // namespace ewfm
'''

    # ---- C++ runtime cpp (skeleton) ----
    files[f"src/devices/{tn}/{cls}Device.cpp"] = f'''#include "devices/{tn}/{cls}Device.h"

#include "devices/core/ConfigCodec.h"

namespace ewfm {{

{cls}Device::{cls}Device(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob) : {base}(/* TODO: initial state */) {{
    (void)decodeValidatedFixedConfigBlob({cfg}::kMagic, configBlob.data(), configBlob.size(), config_);
    bindDeviceIdentity(record, configBlob);
}}

const {cfg}& {cls}Device::config() const {{ return config_; }}
const DeviceBaseConfigV1& {cls}Device::baseConfig() const {{ return config_; }}

bool {cls}Device::serializeConfigBlob(DeviceConfigBlob& configBlob) const {{
    uint8_t buffer[kMaxDeviceConfigBytes]{{}};
    const size_t size = {tn}ConfigSize(config_);
    return encodeFixedConfigBlob({cfg}::kMagic, config_, buffer, size) && configBlob.assign(buffer, size);
}}

bool {cls}Device::applyConfig(const DeviceConfigBlob& configBlob, uint32_t now) {{
    (void)now;
    return decodeValidatedFixedConfigBlob({cfg}::kMagic, configBlob.data(), configBlob.size(), config_);
}}

DeviceTypeDescriptor {cls}Device::descriptor() {{
    DeviceTypeDescriptor descriptor;
    descriptor.typeId = k{cls}TypeId;
    descriptor.name = "{cls}Device";
    descriptor.currentConfigVersion = {m.get('configVersion', 1)};
    descriptor.createRuntime = &{cls}Device::createRuntime;
    descriptor.validateConfig = &{cls}Device::validateConfig;
    // TODO: ticks*, dependencyRequirements, providedRoles.
    return descriptor;
}}

std::unique_ptr<IDeviceRuntime> {cls}Device::createRuntime(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob) {{
    return std::unique_ptr<IDeviceRuntime>(new {cls}Device(record, configBlob));
}}

DeviceValidationResult {cls}Device::validateConfig(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob) {{
    (void)record;
    {cfg} config{{}};
    if (!decodeValidatedFixedConfigBlob({cfg}::kMagic, configBlob.data(), configBlob.size(), config)) {{
        return {{DeviceError::InvalidConfig, "{tn} config is invalid"}};
    }}
    return {{}};
}}

}} // namespace ewfm
'''

    # ---- REST adapter header ----
    files[f"src/integrations/rest/{tn}/{cls}DeviceApiAdapter.h"] = f'''#pragma once

#include "devices/{tn}/{cls}Device.h"
#include "integrations/rest/common/TypedDeviceApiAdapter.h"

namespace ewfm {{

class {cls}DeviceApiAdapter final : public TypedDeviceApiAdapter<{cls}DeviceApiAdapter, {cls}Device, {cfg}> {{
public:
    static constexpr const char* kTypeName = "{tn}";

    void writeRuntimeJson(const {cls}Device& device, JsonObject runtimeJson) const {{
        (void)device;
        (void)runtimeJson;
        // TODO: serialize runtime.output.
    }}
}};

}} // namespace ewfm
'''

    # ---- TS model ----
    ts_fields = "\n".join(f"  {f['name']}: {TS_T[f['kind']]}" for f in fields)
    ts_defaults = "\n".join(f"      {f['name']}: {ts_default(f)}," for f in fields)
    ts_norms = "\n".join(ts_norm(f) for f in fields)
    ts_encodes = "\n".join(f"      {f['name']}: config.{f['name']}," for f in fields)
    draft = f"{cls}ConfigDraft"
    files[f"portal-spa/src/models/devices/{tn.replace('_', '-')}.ts"] = f'''import type {{ DeviceCreateDraftBase }} from '@/models/devices/base'
import {{ BaseDevice, defaultBaseDeviceConfig, normalizeBaseDeviceConfig, encodeBaseDeviceConfig }} from './base-device.ts'
import type {{ BaseDeviceConfig, DeviceRecord }} from '@/api/contracts'

export interface {draft} extends BaseDeviceConfig {{
{ts_fields}
}}

export interface {cls}CreateDraft extends DeviceCreateDraftBase, {draft} {{}}

export class {cls}Device extends BaseDevice<{draft}, {cls}CreateDraft, unknown> {{
  static readonly TYPE_ID = {m["typeId"]} as const
  static readonly TYPE_NAME = '{tn}' as const

  readonly typeName = {cls}Device.TYPE_NAME
  readonly typeId = {cls}Device.TYPE_ID

  static defaultConfig(): {draft} {{
    return {{
      ...defaultBaseDeviceConfig(),
{ts_defaults}
    }}
  }}

  static normalizeConfig(value: unknown): {draft} {{
    const defaults = {cls}Device.defaultConfig()
    if (typeof value !== 'object' || value === null || Array.isArray(value)) {{
      return defaults
    }}
    const raw = value as Record<string, unknown>
    return {{
      ...normalizeBaseDeviceConfig(raw, defaults),
{ts_norms}
    }}
  }}

  static encodeConfig(config: {draft}): Record<string, unknown> {{
    return {{
      ...encodeBaseDeviceConfig(config),
{ts_encodes}
    }}
  }}

  createDefaultConfig(): {draft} {{ return {cls}Device.defaultConfig() }}
  createDefaultCreateDraft(common: Partial<DeviceCreateDraftBase> = {{}}): {cls}CreateDraft {{
    return {{ ...this.createDefaultConfig(), ...common, typeName: common.typeName ?? this.typeName }}
  }}
  createEditDraft(current: DeviceRecord): {cls}CreateDraft {{
    return {{ ...this.normalizeConfig(current.config), typeName: this.typeName }}
  }}
  normalizeConfig(value: unknown): {draft} {{ return {cls}Device.normalizeConfig(value) }}
  normalizeOutput(record: DeviceRecord): unknown {{ return record.runtime }}

  protected override encodeConfig(config: {draft}): Record<string, unknown> {{ return {cls}Device.encodeConfig(config) }}
}}
'''

    # ---- Vue stubs ----
    vrows = "\n".join(
        f'''      <v-col cols="12" sm="6">
        <v-text-field :label="'{f["name"]}'" :model-value="modelValue.{f["name"]}" :readonly="mode === 'view'"
          @update:model-value="update('{f["name"]}', {'Boolean' if f['kind'] == 'bool' else 'Number'}($event))" />
      </v-col>''' for f in fields)
    files[f"portal-spa/src/components/devices/{tn.replace('_', '-')}/{cls}Fields.vue"] = f'''<template>
  <div>
    <v-row density="comfortable">
{vrows}
    </v-row>
  </div>
</template>

<script setup lang="ts">
import type {{ DeviceRecord }} from '@/api/contracts'
import {{ {cls}Device, type {draft} }} from '@/models/devices/{tn.replace('_', '-')}'
import {{ useDraftModel }} from '@/composables/useDraftModel'

const props = defineProps<{{ modelValue: {draft}; device?: DeviceRecord; mode: 'view' | 'edit' | 'create'; busy?: boolean }}>()
const emit = defineEmits<{{ 'update:modelValue': [value: {draft}] }}>()
const {{ update }} = useDraftModel(props, emit)
</script>
'''
    files[f"portal-spa/src/components/devices/{tn.replace('_', '-')}/{cls}Widget.vue"] = f'''<template>
  <div><!-- TODO: {cls} widget body --></div>
</template>

<script setup lang="ts">
import type {{ DeviceRecord }} from '@/api/contracts'
defineProps<{{ device: DeviceRecord; editable?: boolean; dense?: boolean }}>()
</script>
'''
    return files


def registration_snippets(m: dict) -> str:
    tn = m["typeName"]
    cls = m.get("className", camel(tn))
    const = screaming(tn) + "_DEVICE_TYPE_ID"
    label, icon = m.get("label", f"device.type.{camel(tn)[:1].lower()+camel(tn)[1:]}"), m.get("icon", "device")
    return f'''
============================ REGISTRATION LINES ============================
1) src/devices/core/DeviceTypes.cpp -> DeviceTypeRegistry::withDefaults():
     (void)registry.registerDescriptor({cls}Device::descriptor());

2) src/integrations/common/DeviceApiAdapter.cpp -> withDefaults():
     (void)registry.registerAdapter({cls}DeviceApiAdapter::instance());

3) portal-spa/src/models/device-type-ids.ts:
     import {{ {cls}Device }} from './devices/{tn.replace('_', '-')}.ts'
     export const {const} = {cls}Device.TYPE_ID

4) portal-spa/src/components/devices/registry/device-ui-registry.ts:
     const {tn.replace('_', '')[:1] + camel(tn)[1:]}Ui: DeviceUi = {{
       typeId: {cls}Device.TYPE_ID, typeName: {cls}Device.TYPE_NAME,
       labelKey: '{label}', icon: '{icon}',
       fieldsComponent: {cls}Fields, widgetComponent: {cls}Widget,
     }}
     // ...and add [{tn.replace('_', '')[:1] + camel(tn)[1:]}Ui.typeId]: {tn.replace('_', '')[:1] + camel(tn)[1:]}Ui to deviceUiV2ByTypeId

5) portal-spa/src/mock/database.ts -> seedDatabase.devices:
     createDeviceRecord(<id>, '{tn}', 1, {{ /* full config */ }}, {{ /* runtime.output */ }}),

Then run: python3 tools/devicegen/check_registry.py .
===========================================================================
'''


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("manifest")
    ap.add_argument("--out", default="tools/devicegen/scaffold-out")
    a = ap.parse_args()
    m = yaml.safe_load(pathlib.Path(a.manifest).read_text())

    files = render(m)
    outroot = pathlib.Path(a.out)
    for rel, content in files.items():
        dest = outroot / rel
        dest.parent.mkdir(parents=True, exist_ok=True)
        dest.write_text(content)
        print(f"  wrote {dest}")
    print(registration_snippets(m))
    print(f"{len(files)} files scaffolded under {outroot}/ -- review, then move into the tree and paste the registration lines.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
