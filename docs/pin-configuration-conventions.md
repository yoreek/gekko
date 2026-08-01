# Pin/Channel Configuration Conventions

This is an audit of every GPIO/channel field across device configs, taken before a
cleanup pass. It exists because the fields grew independently over time and ended up
using three different, undocumented "not configured" conventions (`-1` on a signed type,
`0xFF` on an unsigned type with an ad hoc constant name, or a plain low integer that just
happens to be a real, wireable pin). This doc is the reference the cleanup pass should
follow. **No firmware code has been changed yet** — this is the inventory and the target
state; see "Planned Fixes" for what the follow-up pass must do.

## Terminology

- **Pin** — a direct ESP32 GPIO number (0-48-ish, board-dependent). Used by devices wired
  straight to the controller (`gpio_switch`, `binary_sensor`, `onewire_bus`, `pixel_strip`,
  `analog_output` (LEDC), `dht11`, `ds1302`, `st7735`, `spi_bus`, `i2c_bus`,
  `analog_port_input`, `cd74hc4067`, the direct-pin HD44780 variant behind
  `lcd1602_pin`/`lcd2004_pin`).
- **Channel** — a bit position (0-7) on a PCF8574 I2C GPIO expander, a completely
  different namespace with its own range. Used by the I2C-backpack HD44780 variant
  behind `lcd1602`/`lcd2004`, and by the standalone `port_expander` device family. Channels
  are out of scope for the pin-sentinel unification below — they already have a single,
  consistent `kHd44780ChannelUnset = 0xFFU` sentinel and don't suffer from the "arbitrary
  low integer" problem this doc is about, since a channel can only ever be 0-7 in the
  first place.

## Target convention

One sentinel, one type, project-wide: **`uint8_t` field, `255` (`0xFFU`) means "not
configured."** No signed pin fields, no per-family sentinel constants with different
names for the same concept.

Every pin field then falls into exactly one of three categories:

### Category 1 — Fixed hardware/module-standard default (real value, never a sentinel)

The default reflects an actual hardware convention, not a guess. Leave these alone;
replacing them with `255` would remove real information and force users to always look
up a number they'd otherwise never need to touch.

| Device.field | Type | Default | Why it's real |
|---|---|---|---|
| `i2c_bus.sdaPin` / `sclPin` | `uint8_t` | `21` / `22` | ESP32's standard I2C pins |
| `spi_bus.sckPin` / `mosiPin` | `uint8_t` | `18` / `23` | ESP32's standard VSPI pins |
| `lcd1602`/`lcd2004` (I2C variant) `rsChannel`/`eChannel`/`d4-d7Channel`/`backlightChannel` | `uint8_t` | `0`/`2`/`4,5,6,7`/`3` | The near-universal PCF8574 LCM1602/2004-IIC backpack wiring — see `Hd44780DisplayDeviceConfigBase.h` and `standardLcd1602Wiring()` in the SPA model. Almost every off-the-shelf module ships wired exactly this way, so defaulting to it is a real convenience, not a placeholder. Already well-commented in code; this doc is the more explicit, cross-referenced description the user asked for. |

### Category 2 — Genuinely optional (device works fine with it unset forever)

`255` is a legitimate, permanent, persistable value here — `validate()` must explicitly
accept it, not just tolerate it as a migration artifact.

| Device.field | Type today | Default today | Target type | Notes |
|---|---|---|---|---|
| `spi_bus.misoPin` | `int16_t` | `-1` | `uint8_t`, `255` | Write-only SPI peripherals don't wire MISO. **Type change required** (see Planned Fixes). |
| `st7735.resetPin` | `int8_t` | `-1` | `uint8_t`, `255` | Many ST7735 modules tie RESET to EN/3.3V instead of a GPIO. **Type change required.** |
| `hd44780` (direct-pin) `backlightPin` | `uint8_t` | `kGpioPinUnset` (`0xFF`) | unchanged | Already correct. Confirmed genuinely optional: some HD44780 boards have no separate backlight control line at all (tied permanently high), so there's nothing to set even in principle — not every board *can* offer this pin, matching the confirmed intent. |
| `cd74hc4067.enablePin` | `uint8_t` | `kGpioPinUnset` (`0xFF`) | unchanged | Already correct — module's `~E` pin is commonly tied to GND. |

### Category 3 — Mandatory, but today's default is an arbitrary "junk" number

The device cannot function without a real pin here, and there is no hardware-standard
value to default to — the current default is just whatever number the original author
picked. Target: default to `255`, and `validate()` must *reject* `255` (and any other
out-of-range value), so a device can never be saved/created without the user explicitly
choosing a real pin. This is a "force explicit choice" sentinel, not a "may stay unset"
one — different from Category 2 even though it reuses the same literal value.

| Device.field | Type | Current default | Validation today | Gap |
|---|---|---|---|---|
| `gpio_switch.gpioPin` | `uint8_t` | `2` | `gpioSwitchPinIsValid()` already rejects `255` | none — just swap the default literal |
| `binary_sensor.gpioPin` | `uint8_t` | `4` | `binarySensorPinIsValid()` already rejects `255` | none |
| `dht11.gpioPin` | `uint8_t` | `17` (`kDht11DefaultGpioPin`) | reuses `gpioSwitchPinIsValid()`, already rejects `255` | none |
| `ds1302.clkPin` / `dataPin` / `rstPin` | `uint8_t` ×3 | `25` / `26` / `27` | reuses `gpioSwitchPinIsValid()` per field, already rejects `255` | none |
| `onewire_bus.gpioPin` | `uint8_t` | `4` | **`validate()` does no range check at all** | real gap — needs a range/validity check added, not just a default swap |
| `analog_port_input.gpioPin` | `uint8_t` | `34` | `analogPortInputGpioPinIsValid()` (exact ADC1 pin whitelist) already rejects `255` | none |
| `cd74hc4067.sigPin` | `uint8_t` | `34` | same ADC1 whitelist, already rejects `255` | none |
| `st7735.chipSelectPin` / `dcPin` | `uint8_t` ×2 | `5` / `2` | **V5's `validate()` doesn't check these at all** (dropped somewhere between V1-V4 and V5 — V1-V4 had `validateSt7735DeviceConfigV*` helpers, V5 only checks `spiBusDeviceId`/`rotation`/`panel`/dimensions) | real gap — needs the range check reinstated |
| `hd44780` (direct-pin) `rsPin`/`ePin`/`d4Pin`/`d5Pin`/`d6Pin`/`d7Pin` | `uint8_t` ×6 | **`0` for all six** | `gpioSwitchPinIsValid()` per pin, plus a distinctness check across all 7 (incl. `backlightPin`) | **currently broken by construction**: all six defaults are equal (`0`), so a freshly-created device with untouched defaults fails the "pins must be distinct" check immediately. Target: all six default to `255` (matches the SPA's `lcd1602-pin.ts`/`lcd2004-pin.ts`, which already send `255` for these — the SPA got ahead of the firmware default here). The distinctness check already treats `kGpioPinUnset` as "skip," so 6×`255` passes cleanly through `validate()` up front and simply requires the user to fill in 6 distinct real pins before save, same as `gpio_switch` et al. |

## Planned fixes (next pass, not yet implemented)

1. **`spi_bus.misoPin`**: `int16_t → uint8_t`, `-1 → 255`. Changes field type/size ⇒ per
   `docs/device-config-versioning.md`, requires a new `SpiBusDeviceConfigV2` with
   `migrateFrom()` mapping old `< 0` → `255`, old `>= 0` passthrough as `uint8_t`.
2. **`st7735.resetPin`**: `int8_t → uint8_t`, `-1 → 255`. No migration needed, unlike
   `misoPin` — both types are 1 byte, same alignment, and `-1` as `int8_t` is already the
   bit pattern `0xFF`, which reads back as `255` under `uint8_t` with zero reinterpretation
   needed. The binary layout is identical before and after; this is a straight in-place
   type relabel on the existing struct, not a version bump.
3. **`st7735.chipSelectPin`/`dcPin`**: add the missing range check to `V5::validate()`.
   No version bump involved (same reasoning as item 4 — this doesn't touch field
   shape/type/meaning, just tightens a runtime check). The one real risk: `validate()`
   also gates decode (`decodeValidatedFixedConfigBlob` calls `validate()` and rejects on
   failure), so if any already-persisted device somehow has an out-of-range value in
   these fields today (never checked before), tightening the check would make that
   device fail to load on next boot. Worth a quick check before shipping, not a reason to
   avoid the fix.
4. **Category 3 default-literal swaps** (`gpio_switch`, `binary_sensor`, `dht11`,
   `ds1302`, `analog_port_input`, `cd74hc4067.sigPin`, `hd44780` direct-pin rs/e/d4-d7):
   **no version bump needed.** Versioning exists to keep already-persisted binary blobs
   decodable — it's triggered by changes to field shape/name/type/meaning, since those
   change how existing bytes are interpreted. A field's C++ default-member literal isn't
   part of the persisted bytes at all: decoding an old blob reads the actual stored value,
   never the default, and every real create/update request sends an explicit JSON value
   that overwrites the default immediately via `parseJson`. The default only matters for
   the moment between default-construction and `parseJson` — nothing persisted ever
   observes it. So these are plain literal edits in the existing `*ConfigV<n>` struct, no
   new `kMagic`/`migrateFrom`/decode fallback/migration tests required.
5. **`onewire_bus.gpioPin`**: add a real range/validity check (currently none at all) —
   independent bug fix, do alongside the default swap.
6. Keep the SPA's `defaultConfig()` for each affected type in sync with whatever the
   firmware ends up defaulting to, the same way `lcd1602-pin.ts`/`lcd2004-pin.ts` already
   default to `255` for their six mandatory pins today.

## Related Specs

- [Device Config Versioning](device-config-versioning.md) — the versioning rules that
  gate items 1, 2, and 4 above
- [Device Model Structures](device-model-structures.md)
