# Device Config Versioning

Device config structs are persisted binary formats. Treat every released `*DeviceConfigV*` struct as immutable once it can exist in NVS or an imported setup bundle.

## Compiler-Enforced Legacy Guard

Old `*ConfigV<n>` structs must never be a runtime's or adapter's *active* config — only
decode/migration may name them. This is enforced two ways:

- **Compiler (authoritative):** mark each superseded struct `[[deprecated("...")]]`. The
  esp32dev build passes `-Werror=deprecated-declarations` via `build_src_flags` (scoped
  to `src/`, so third-party libraries are unaffected), so any active use is a hard build
  error. Legitimate migration/decode/`static_assert`/ctor code opts out by wrapping the
  region in `EWFM_LEGACY_CONFIG_USE_BEGIN` … `EWFM_LEGACY_CONFIG_USE_END` (defined in
  `devices/core/ConfigCodec.h`). `ds18b20` is the reference (`Ds18b20TemperatureSensorConfigV1`).
- **Fast pre-check:** `tools/devicegen/check_config_versions.py` runs in `scripts/test.sh`
  and flags the same misuse by static scan (covers every family without needing the
  attribute). See `docs/device-scaffolding.md`.

When you add a new config version, mark the previous one `[[deprecated]]` and wrap the
migration/decode sites; deleting now-dead `parseJson`/`writeJson` on the old struct
(only `validate` is needed for decode) keeps the guarded surface small.

## Core Rules

- Never edit, rename, reorder, remove, or reinterpret fields in an existing `*DeviceConfigV*` struct.
- When a persisted field changes shape, name, type, meaning, or default, create the next `*DeviceConfigV*` struct.
- Keep old config structs available for decode and migration tests.
- Runtime code, REST adapters, setup transfer import/export, and new writes must use only the latest config struct.
- `encode*DeviceConfig(...)` writes only the latest binary format.
- `decode*DeviceConfig(...)` returns the latest config struct and migrates older supported blobs internally.
- `descriptor().currentConfigVersion` must match the latest struct version.
- The binary marker, usually `kMagic`, identifies the real persisted layout. Do not rely on public JSON field names for binary compatibility.

## Documented Exceptions

`Lcd1602DeviceConfigV2`/`Lcd2004DeviceConfigV2` (`Hd44780DisplayDeviceConfigBase`,
`src/devices/display/hd44780/Hd44780DisplayDeviceConfigBase.h`) were edited in place — channel
fields and the base struct changed from a switch-dependency-slot model to embedded PCF8574 I2C
channels — without a version bump, and `kMagic` ("LCD1602-2"/"LCD2004-2") stayed the same. This is
a deliberate, one-time exception to the Core Rules above: the SPA never sent a create request the
backend's old dependency validation would accept (it sent a single `port_expander` dependency where
6-7 ordered `switch` deps were required), so no device of this shape was ever created end-to-end —
there is no real persisted config to preserve or migrate from.

## Migration Pattern

When changing a config format:

1. Leave the old struct unchanged.
2. Add the new struct with a new `kMagic` marker.
3. Move `parseJson`, `validate`, and `writeJson` to the latest struct.
4. Add `migrateFrom(const OldConfigVn&)` on the latest struct or an equivalent local migration helper.
5. Update `decode*DeviceConfig(...)` to try the latest format first, then old supported formats.
6. Update runtime members, REST adapters, setup transfer codec, and tests to use the latest type.
7. Add/keep tests for current round-trip and every supported old-to-current migration.

Example:

```cpp
struct Ssd1306DeviceConfigV1 : DeviceBaseConfigV1 {
    static constexpr char kMagic[] = "OLV1";
    uint16_t layoutWidth{128};
    uint16_t layoutHeight{64};
};

struct Ssd1306DeviceConfigV2 : DeviceBaseConfigV1 {
    static constexpr char kMagic[] = "OLV2";
    uint16_t width{128};
    uint16_t height{64};

    void migrateFrom(const Ssd1306DeviceConfigV1& oldConfig);
};
```

After this change, `layoutWidth` and `layoutHeight` may appear only in `Ssd1306DeviceConfigV1`, migration code, and migration tests. Runtime and REST use `width` and `height`.

## REST And JSON

REST JSON is the current public contract, not a binary migration layer.

- REST create/update parsers accept current field names only.
- Do not keep fallback parsing for old JSON field names unless a specific setup-bundle import requirement says so.
- If an old field name would otherwise be silently ignored and defaulted, reject it explicitly.
- Device JSON snapshots must serialize only current field names.

## Tests

For each versioned config change, cover:

- current encode/decode round-trip;
- old binary blob decode and migration to latest;
- registry boot migration when old blobs may be stored in NVS;
- REST rejection of removed/renamed JSON fields when those fields are not part of the current API;
- REST serialization of the current field names.

## Related Specs

- [Device Model Structures](device-model-structures.md)
