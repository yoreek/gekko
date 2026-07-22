# Device Scaffolding

How to add a new device type, and the manifest-driven tooling proposed to make it
cheaper. Read [device-config-versioning.md](device-config-versioning.md) and
[rest-api-contract.md](rest-api-contract.md) first — this doc coordinates the pieces
they define, it does not replace them.

> **Status.** The **manual checklist** below is the process today and is authoritative.
> The **manifest / field-kind codegen** (from "Manifest model" onward) is a *proposal
> with a working pilot* (`tools/devicegen/`, gpio_switch only). Do not assume a
> generator exists for a type until its `*.device.yaml` is committed and wired into
> `scripts/test.sh`. Until then, add types by hand using the checklist.

## Why this exists

A device type is spread across ~10 registration points in two codebases. The same
field list is retyped in the C++ struct, the C++ JSON codec, the TypeScript contract,
the Vue form, and the mock seed. Nothing enforces that these agree — e.g. the pilot's
consistency check found `gpio_switch`'s default pin declared as `2` in the C++ struct
(`GpioSwitchDevice.h`) but `4` in the TS `defaultConfig` (`gpio-switch.ts`). The goal
is to (a) make the touchpoints mechanical and (b) make divergence a failing test.

## Adding a device type today (manual checklist)

Work through every item. Missing one usually fails at runtime, not at build.

**Firmware**

1. **Config struct** — `*DeviceConfigV*` with a `kMagic` marker and `parseJson` /
   `writeJson` / `validate`. Compose from a family base where one exists
   (`SwitchDeviceConfigV2`, `OutputDeviceConfigV1<T>`, `I2cDeviceConfigV1`, …) and
   declare only the new fields. Binary encoding is automatic for trivially-copyable
   structs via `ConfigCodec.h` — you do not hand-write serialization. Follow
   [device-config-versioning.md](device-config-versioning.md) for any later change.
2. **Runtime class** — derive from `DeviceRuntimeBase` (or a family base such as
   `SwitchDeviceBase`, `AnalogOutputDeviceBase`) and implement a static
   `descriptor()` returning the `DeviceTypeDescriptor` (typeId, config version,
   provided/required roles, tick flags, `createRuntime`, `validateConfig`).
3. **Register the descriptor** — add one line to `DeviceTypeRegistry::withDefaults()`
   in `src/devices/core/DeviceTypes.cpp`.
4. **REST adapter** — subclass `TypedDeviceApiAdapter<Derived, Device, Config>` under
   `src/integrations/rest/<type>/`; override only the hooks that differ (usually just
   `writeRuntimeJson`). Register it in `DeviceApiAdapterRegistry::withDefaults()`
   (`src/integrations/common/DeviceApiAdapter.cpp`). See
   [controller-ruleschain.md](controller-ruleschain.md).
5. **(Optional) Home Assistant adapter** — under `src/integrations/mqtt/<type>/`,
   guarded by `#ifdef WITH_HOME_ASSISTANT`, registered in
   `HaEntityAdapterRegistry::withDefaults()`. This registry is independent by design —
   HA may not compile at all — so it is never merged with the REST/core registries.

**Frontend (`portal-spa/`)**

6. **Model** — `src/models/devices/<type>.ts`: a `BaseDevice` subclass with
   `TYPE_ID` / `TYPE_NAME`, `defaultConfig()`, `normalizeConfig()`, and the config
   draft interface. Reuse shared drafts (`SwitchConfigDraft`, …) to mirror the C++
   composition.
7. **Type id** — register in `src/models/device-type-ids.ts`.
8. **UI registry** — add a `DeviceUi` entry and import the components in
   `src/components/devices/registry/device-ui-registry.ts`.
9. **Components** — `<Type>Fields.vue` (type-specific fields only; base name/enabled/
   deps are rendered by `DeviceBaseFields`) and `<Type>Widget.vue` (one component,
   dense + expanded). Vuetify only — see the UI rules in `CLAUDE.md`.
10. **Mock seed** — a fully-configured instance in `src/mock/database.ts`
    (`seedDatabase.devices`) so `?mockMode=1&mockReset=1` exercises the type.
11. **i18n** — a `device.type.<x>` label key (and any field labels).

Keep the same `typeName` string and `typeId` value across the adapter, the C++
descriptor, and the TS model — that identity is the contract.

## Base classes as templates

The cheapest a new device can be is when a family base already owns its runtime shape,
leaving the concrete class to fill in a few hooks. This is stronger leverage than
codegen for the parts codegen cannot touch (the runtime loop, not the JSON surface).
Pick the closest base instead of deriving straight from `DeviceRuntimeBase`:

| Family | Base | Concrete class supplies |
|---|---|---|
| Root machinery | `DeviceRuntimeBase` | lifecycle/deps/status/persistence — inherited by all |
| Switch / analog output | `AbstractOutputDevice<T>` → `SwitchDeviceBase` / `AnalogOutputDeviceBase` (+ decorator base) | `config()`/`mutableConfig()` + hardware write |
| Device on a bus | `BusDependentDeviceBase<Bus, Role>` | bus access |
| Hub channel | `AnalogInputHubChannelDeviceBase` | protocol driver |
| **Polled temperature sensor** | **`PolledTemperatureSensorDeviceBase`** | `sensorReady()`, `sampleReading()`, poll/filter/report accessors |

`PolledTemperatureSensorDeviceBase` owns the whole lifecycle state machine, poll
cadence, smoothing filter, and reading publisher. It was extracted because
`ntc_thermistor`, `ds18b20`, and `htu21` each hand-rolled the same ~130-line state
machine (htu21 had even grown its own private base). Migrating `ntc_thermistor` onto
it dropped that class from 408 to 216 lines with no behavior change (all native tests
pass).

`ds18b20` and `htu21` do **not** fit this base, despite also being temperature
sensors: both derive from `BusDependentDeviceBase` (a diamond over `DeviceRuntimeBase`
with `PolledTemperatureSensorDeviceBase`), read asynchronously across several states
(trigger → wait for conversion → read, plus fault/retry), and `htu21` is dual-channel
(temperature + humidity, two publishers/filters). They form a separate "async bus
sensor" sub-family that would need its own base — a reminder to size a base to one
read model, not to every device that shares a role. `ds18b20` did gain the same
smoothing/calibration filter as a standalone config-version bump (`DS18B20-2`) for
parity, without adopting the base.

When a family has no base yet and a second device is about to duplicate a first,
extract the base *before* the copy — that is the highest-leverage move for a family
you expect to grow (sensors above all). The scaffolder should pick the base by family;
`check_registry.py` can later assert e.g. "temperature-sensor types derive from
`PolledTemperatureSensorDeviceBase`".

## Manifest model (proposed)

The unit of description is a **field `kind`**, not a raw type. A `kind` is a bundle
that generates across every layer at once — C++ parse/write/default, TS type/default/
normalize, and optionally a Vuetify field — which is what removes the divergence class
above. One `<type>.device.yaml` per device:

```yaml
typeName: gpio_switch
typeId: 2
configVersion: 3
label: device.type.gpioSwitch
icon: power

extends: [switch]                 # composition, not a flat list: mirrors the C++
                                  # config inheritance; declare only the delta

roles:
  provides: [switch, condition]

fields:
  - { name: gpioPin, kind: pin, default: 4 }          # kind from the closed vocabulary
  # thermostat-style examples:
  # - { name: mode,   kind: enum, enum: ThermostatMode }
  # - { name: target, kind: temperature, json: targetCelsius, store: milliCelsius }
  # - { name: checkInterval, kind: duration, min: 100, max: 86400000 }

groups:                           # optional nesting for the few types that need it
  # container: { fields: [ { name: capacityMl, kind: uint, min: 1, max: 65535 } ] }

ui:
  fields: handwritten             # or `generated` to emit a Vuetify form from `fields`
  widget: handwritten

runtime: handwritten              # hardware/ticks are always hand-written
```

### Composition, not flat fields

Configs already inherit (`SwitchDeviceConfigV2 → OutputDeviceConfigV1 → …`). The
manifest must express that with `extends`/`mixin` and declare only new fields, or the
shared surface gets duplicated and can drift. The generator emits the base delegation
(`Base::parseJson(...)`) and appends the delta.

### Field-kind vocabulary

A closed set, each mapping to a helper that already exists in the codebase. Add a
`kind` when a pattern recurs (≥2 uses); do not add per-field special cases to the
generator.

| kind | maps to | notes |
|---|---|---|
| `uint` / `int` / `float` | `parseUint8` / `parseInt` / `parseFloatField` | with `min`/`max` |
| `bool` | `parseBoolField` | |
| `enum` | `<Enum>FromString` / `<Enum>FromByte` | JSON is a string |
| `duration` | `parseDuration` | ms, `min`/`max` |
| `temperature` | `parseTemperatureField` | `json` key + `store` unit (milli/centi) |
| `pin` | `gpioSwitchPinIsValid`-style guard | |
| `i2cAddress` | address validation | |
| `depRef` | dependency id + `isNull()` handling | references another device by role |
| `string` | `strlcpy` / `copyString` | with `maxLen` |
| `custom` | a hand-written hook | generator emits the call, you write the body |

### Escape hatch (two granularities)

- **Per field:** `kind: custom` — the generated `parseJson` calls a hand-written hook
  (`<type>ConfigValidateField_<name>`), everything else for that field is skipped.
- **Per section / whole type:** `parseJson: handwritten` (or `ui.fields: handwritten`,
  `runtime: handwritten`) — the generator skips that layer entirely.

## Which types the generator covers

Derived from a scan of all `parseJson` implementations. Roughly three-quarters of
types are fully coverable by the vocabulary; the rest are bespoke and opt out.

- **Regular (covered):** gpio/expander/auto switches, binary sensor, ds18b20, htu21,
  i2c/spi/onewire buses, ledc/fade analog outputs, analog input channel + ADS1115/
  CD74HC4067 hubs, analog port input, PCF857x expanders, RTC, displays. These are
  scalar / bool / enum / pin / address / duration / depRef fields only.
- **Regular + kinds:** thermostat — needs `temperature`, `duration`, `enum`.
- **Bespoke (mostly hand-written):** dosing_pump (nested `container`/`schedule`
  objects, `doses` array, float→fixed transforms), schedule (`rules` array),
  scheduled / composer analog outputs (schedule graph), ntc_thermistor (curve/
  presets). Set `parseJson: handwritten` and keep these as they are.

A field's JSON shape rarely maps 1:1 to the struct on bespoke types — that mismatch
(nesting, arrays, unit transforms) is the boundary between generated and hand-written.

## Tooling & how to use it

Lives in `tools/devicegen/`.

### `check_registry.py` — all-types consistency guard (in CI)

Runs in `scripts/test.sh`. Manifest-free: takes the REST adapters' `kTypeName` as the
canonical type set and verifies, for all types at once, that each is registered in
`DeviceApiAdapterRegistry::withDefaults()`, has a matching TS model `TYPE_NAME`, is
referenced in `device-ui-registry.ts`, and has at least one mock seed. Also flags
reverse orphans (TS/mock type with no adapter) and duplicate TS `TYPE_ID`s.

```sh
python3 tools/devicegen/check_registry.py .    # non-zero exit on any problem
```

This is the cheap, always-valuable half — it does not depend on any field being
trivial, so it covers every type including the bespoke ones. It does **not** check
field-level defaults; that needs a per-type manifest (below).

### `generate.py` — field-surface pilot (manifest-driven, `gpio_switch` only)

```sh
# emit the generated field-surface for review
python3 tools/devicegen/generate.py tools/devicegen/devices/gpio_switch.device.yaml generate

# check a manifest against the committed code; non-zero exit on divergence
python3 tools/devicegen/generate.py tools/devicegen/devices/gpio_switch.device.yaml check --repo .
```

The manifest `check` mode adds per-field default agreement between the C++ struct, the
TS `defaultConfig`, and the manifest — the check that caught the `gpioPin` 2/4 drift.
Requires `pyyaml`. Only `gpio_switch` has a manifest today.

## Roadmap

1. **Consistency guard — done.** `check_registry.py` covers all types and gates
   `scripts/test.sh`.
2. **Scaffolder — next.** Stamp the checklist's files and registration lines from a
   manifest so a new type starts wired-up. Field-kind independent.
3. **Field-kind codegen.** Generate the field surface for regular types from the
   manifest so C++ and TS cannot drift. Worth building once the flow of regular types
   justifies it; bespoke types stay hand-written via the escape hatch.
