# Device Config Versioning

Device config structs are persisted binary formats. Treat every released `*DeviceConfigV*` struct as immutable once it can exist in NVS or an imported setup bundle.

## Core Rules

- Never edit, rename, reorder, remove, or reinterpret fields in an existing `*DeviceConfigV*` struct.
- When a persisted field changes shape, name, type, meaning, or default, create the next `*DeviceConfigV*` struct.
- Keep old config structs available for decode and migration tests.
- Runtime code, REST adapters, setup transfer import/export, and new writes must use only the latest config struct.
- `encode*DeviceConfig(...)` writes only the latest binary format.
- `decode*DeviceConfig(...)` returns the latest config struct and migrates older supported blobs internally.
- `descriptor().currentConfigVersion` must match the latest struct version.
- The binary marker, usually `kMagic`, identifies the real persisted layout. Do not rely on public JSON field names for binary compatibility.

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
