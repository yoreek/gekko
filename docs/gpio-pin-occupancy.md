# GPIO pin occupancy table

## What this is

A firmware-authoritative table answering "which device currently owns GPIO N", maintained
incrementally by `DeviceRegistry` and (eventually) exposed over REST so the SPA's `PinPicker` can
show/disable already-occupied pins without maintaining its own guess at which config fields are
pins per device type.

## Why firmware-side, not SPA-side

An earlier attempt kept a hand-maintained `typeName -> pin field names` map in the SPA
(`PIN_FIELDS_BY_TYPE`). Rejected: it duplicates knowledge the firmware already has per-type
(`docs/pin-configuration-conventions.md`'s field inventory), and can silently drift when a device
type's fields change or a new type is added. Firmware is the single source of truth for what each
device's config actually contains; the SPA should just consume the result.

## Storage

`DeviceRegistry` holds a fixed array:

```cpp
DeviceId pinOwner_[kGpioPinTableSize]{};  // index = raw GPIO number, value = owning DeviceId, 0 = free
```

`kGpioPinTableSize` (`src/devices/core/DeviceTypes.h`) is a compile-time constant: the compiled
chip's maximum GPIO number + 1 (not `kChipPinCount`, since GPIO numbers aren't contiguous -- there
are gaps, e.g. GPIO 20/24/28-31 don't exist on ESP32). It never varies at runtime -- board
selection (`BoardModel`) is a runtime setting, but every currently-supported board is built on the
same compiled chip, so the table size is fixed for a given firmware binary. Indexing directly by
GPIO number avoids needing a separate "GPIO number -> table position" lookup. No heap allocation,
no `std::vector` -- a plain fixed-size member, same discipline as the rest of `DeviceRegistry`.

## How a device reports its own pins

Two virtual methods on `IDeviceRuntime` (`src/devices/core/DeviceTypes.h`), next to the existing
per-type resource-reporting virtuals (`spiChipSelectPin`, `expanderChannel`, `dependencySlots`):

```cpp
virtual void claimGpioPins(DeviceId* pins) const {}
virtual void releaseGpioPins(DeviceId* pins) const {}
```

`pins` is always `kGpioPinTableSize` long -- since that's a single global constant, it isn't passed
as a parameter (no per-call variation to account for). Default: no-op (types with no physical GPIO
pins -- buses/hubs addressed purely via dependency, `schedule`, `dosing_pump`, etc.). A pin-owning
type overrides both, reading its own **already-live** `config_` (no decoding/copying a blob -- the
runtime already holds the config by value/reference) and writing directly into the caller-owned
table via the shared `setGpioPinOwner()` helper:

```cpp
void GpioSwitchDevice::claimGpioPins(DeviceId* pins) const {
    setGpioPinOwner(pins, config_.gpioPin, deviceId());  // IDeviceRuntime::deviceId() -- already knows its own id
}

void GpioSwitchDevice::releaseGpioPins(DeviceId* pins) const {
    setGpioPinOwner(pins, config_.gpioPin, 0);
}
```

`setGpioPinOwner()` itself checks `gpio != kGpioPinUnset && gpio < kGpioPinTableSize` before
writing -- this transparently skips "not configured" sentinel values (255/`0xFF`) for
genuinely-optional pin fields (`spi_bus.misoPin`, `st7735.resetPin`, HD44780 `backlightPin`,
`cd74hc4067.enablePin`) too, so callers never need a separate sentinel check.

Multi-pin types (`cd74hc4067_hub`, `rtc_ds1302`, `i2c_bus`, `spi_bus`, `lcd1602_pin`/`lcd2004_pin`,
`st7735`) call `setGpioPinOwner()` once per pin field they own.

## Where `DeviceRegistry` calls these

Three call sites, all already handling the relevant runtime/config transition:

1. **`reloadRuntimeFor()`** (`DeviceRegistry.cpp`) -- the single function used both for loading every
   persisted device at controller boot and for `create()`-ing a new device. After the runtime is
   built: `entry.runtime->claimGpioPins(pinOwner_)`. This means the table is naturally rebuilt
   from scratch on every boot from real persisted configs -- no separate "restore on startup" logic
   needed, and no stale state survives a restart.
2. **`updateConfigAndDeps()`** -- `descriptor`/`currentRuntime` are already in scope. Before
   `applyConfig()` (while the runtime still holds the *old* config): `currentRuntime->releaseGpioPins(pinOwner_)`.
   After `applyConfig()` succeeds (same runtime object, now holding the *new* config):
   `currentRuntime->claimGpioPins(pinOwner_)`.
3. **`remove()`** -- before `clearRuntime(deviceId)`: `currentRuntime->releaseGpioPins(pinOwner_)`.

## Board selection is irrelevant here

Switching the selected `BoardModel` (see the board-catalog feature) never touches this table.
Occupancy is indexed by raw GPIO number, a chip-level fact; validation of pin roles is already
chip-scoped, not board-scoped (an earlier, separate design decision). The board selection only
affects which pin subset/labels the SPA offers going forward -- it does not retroactively affect
already-configured devices or this table.

## REST exposure

`GET /api/system/pins` (`PinOccupancyController`, modeled on `BoardController::index()`): flat JSON
built directly from `DeviceRegistry::pinOwners()`, no per-device envelope --
`{"success":true,"pins":[{"gpio":5,"deviceId":12},...]}`, listing only occupied pins. Schema at
`schemas/rest/v1/responses/system-pins.response.schema.json`, smoke-tested by
`test_system_pins_response_schema_smoke` (`test/test_portal/test_portal_api_schemas.cpp`).

## SPA consumption

`usePinOccupancyStore` (`portal-spa/src/stores/pinOccupancy.ts`) refetches on every `PinPicker`
mount -- unlike the board-settings store (rarely changes, cached per session), occupancy changes
whenever *any* device anywhere is created/reconfigured/removed, so it can't be session-cached
without going stale as soon as a second device form opens. `PinPicker.vue` disables a pin already
claimed by another device, with a subtitle naming the owner (reuses the existing
`device.dialog.addressOccupiedBy` i18n key, same pattern already used by `I2cAddressPicker`'s
address-occupancy UI).

## Same-device sibling-pin distinctness

Cross-device occupancy (above) doesn't catch a *single* device's own multiple pin fields
colliding with each other (e.g. st7735's `chipSelectPin` and `dcPin` both set to GPIO5). Handled
independently on both sides:

- **Firmware**: each multi-pin type's `validate()` rejects duplicate pins among its own fields,
  skipping fields holding `kGpioPinUnset`. Already existed for `rtc_ds1302`, `spi_bus`, `i2c_bus`,
  `cd74hc4067_hub` (the latter via a local `pinsOverlap()` helper); added for `st7735`
  (`St7735DeviceConfigV6::validate()`) and the shared `Hd44780PinDisplayDeviceConfigBase` used by
  both `lcd1602_pin`/`lcd2004_pin`.
- **SPA**: `PinPicker`'s new `siblingPins?: number[]` prop -- the *other* pin fields' current draft
  values on the same form (excluding the field's own value and `PIN_UNSET`) -- disables a pin with
  the `pinRole.usedByAnotherField` i18n key, proactively, before the user even hits Save. Wired
  into all 6 multi-pin forms: `St7735Fields.vue`, `Lcd1602PinFields.vue`/`Lcd2004PinFields.vue`,
  `RtcDs1302Fields.vue`, `SpiBusFields.vue`, `I2cBusFields.vue`, `Cd74hc4067HubFields.vue`.

## Status

Fully implemented end to end: firmware table + REST endpoint + SPA occupancy UI + same-device
sibling-pin distinctness (firmware validation and proactive SPA UI). No remaining work identified.
