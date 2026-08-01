# ESP32 Board Models — Research Database

Organized by **chip variant**, because that is what actually determines a
compile target: the firmware toolchain and architecture (Xtensa vs RISC-V) is
fixed per chip, not per physical board. Every board listed under the same
chip already runs (or would run) the exact same compiled binary — they only
differ in which GPIOs are physically broken out and what's permanently wired
to what. List gathered from vendor pinout references, the ESPHome board
registry (`esphome/components/esp32/boards.py`), the Tasmota
supported-device/template repository (templates.blakadder.com), and
espboards.dev's board catalog (272 boards across 6 chip families at the time
of writing — this doc picks a handful of genuinely distinct/popular boards
per family, not an exhaustive catalog).

**Current state: all 5 candidate chips compile and are wired into
`platformio.ini` as real environments** (`esp32dev`/`esp32dev_ble`/
`esp32dev_ota` for classic, plus `esp32s2`, `esp32s3`/`esp32s3_ble`,
`esp32c3`/`esp32c3_ble`, `esp32c6`). **H2 has no WiFi radio at all** and
cannot run this project's portal architecture regardless of pin work — not
included as a target.

## Compile results (2026-08-01)

| Chip | Env | Platform | RAM | Flash |
|---|---|---|---|---|
| Classic ESP32 | `esp32dev` | official `platformio/espressif32@6.9.0`, core 2.x | 17.9% (58,528 / 327,680 B) | 58.8% (1.85 MB / 3 MB `app0`) |
| S2 | `esp32s2` | same, core 2.x | 16.2% (53,160 / 327,680 B) | 56.4% |
| S3 | `esp32s3` | same, core 2.x | 17.8% (58,196 / 327,680 B) | 57.1% |
| C3 | `esp32c3` | same, core 2.x | 15.9% (52,020 / 327,680 B) | 55.9% |
| C6 | `esp32c6` | `pioarduino` fork, core 3.x | 17.6% (57,540 / 327,680 B) | 47.2% (1.98 MB / 4 MB `app0` — C6's board has a 4 MB partition table, same layout, more total flash headroom than the 3 MB `app0` slot on the other four) |

### `platformio.ini` structure: BLE off by default, only turned on where it works

Every chip env is built BLE-off by default (`[env]`'s top-level `lib_ignore =
WiFiProv, SimpleBLE`, no `-DWITH_BLE_PROVISIONING` flag anywhere in the base
`build_flags`). A chip's `_ble` variant `extends` the plain env and only adds
the flag back plus un-ignores the two libraries (`lib_ignore =` empty) — a
few lines, nothing declared then later undone. `env:esp32dev` itself now
carries every classic-ESP32 platform/board/toolchain setting directly (no
longer a child of a separate `_ble`-named base); `esp32dev_ble` and
`esp32dev_ota` both simply `extend` it.

### BLE-provisioning compile results per chip

| Chip | `_ble` env | Result | RAM | Flash |
|---|---|---|---|---|
| Classic ESP32 | `esp32dev_ble` | ✅ SUCCESS | 21.9% (71,760 B) | 83.2% (2.62 MB) |
| S3 | `esp32s3_ble` | ✅ SUCCESS | 24.2% (79,316 B) | 75.0% (2.36 MB) |
| C3 | `esp32c3_ble` | ✅ SUCCESS | 22.3% (73,236 B) | 77.0% (2.42 MB) |
| S2 | *(none)* | ❌ impossible | — | — |
| C6 | *(none)* | ❌ blocked | — | — |

BLE costs a real, consistent ~6–20 percentage points of flash across every
chip that supports it (the whole reason this project ships two separate
binaries instead of one BLE-always-on build) — confirmed by these numbers,
not assumed.

**S2 has no BLE variant because it has no Bluetooth/BLE radio at all** — it's
a WiFi-only chip. `framework = arduino` for `esp32-s2-saola-1` with
`-DWITH_BLE_PROVISIONING` fails to *link*, not compile:
`undefined reference to 'wifi_prov_scheme_ble'` /
`'wifi_prov_scheme_ble_set_service_uuid'` — the SDK simply never builds that
symbol for this chip. Confirmed by an actual build attempt, not inferred
from the datasheet.

**C6 has no BLE variant because of a framework packaging bug**, not
something in this project's own source: linking `esp32c6` with
`-DWITH_BLE_PROVISIONING` on the `pioarduino` core-3.x release fails with

```
multiple definition of `wi_fi_scan_result__init'
(and 5+ more protobuf symbols)
```

between `libespressif__network_provisioning.a` and `libwifi_provisioning.a`
— two internal ESP-IDF components in this Arduino-ESP32 3.x release that
both compile the same generated protobuf code and end up in the final link.
Not investigated further (e.g. `-Wl,--allow-multiple-definition` as a rough
workaround was not tried) — recorded as a known blocker for a future pass if
BLE provisioning on C6 becomes a real requirement.

No chip is a memory risk — this supersedes the paper estimate further below
that called S2 the "tightest candidate"; its real numbers are in line with
(slightly better than) classic ESP32.

### Classic/S2/S3/C3: same official platform, same Arduino-ESP32 core 2.x

All four build against the platform already pinned in `platformio.ini`
(`platformio/espressif32@6.9.0`). Each needs its own toolchain pinned
explicitly — the platform's own default per-board toolchain is an older GCC
`8.4.0` that miscompiles this codebase (see below) — plus a couple of
chip-specific flags:

| Chip | Board used | `board_build.mcu` | Toolchain package pinned | Extra flags needed |
|---|---|---|---|---|
| S3 | `esp32-s3-devkitc-1` | `esp32s3` | `toolchain-xtensa-esp32s3@12.2.0+20230208` | `board_build.f_flash=80000000L`, `board_build.flash_mode=qio` (the board's own defaults — `env:esp32dev_ble`'s inherited 40 MHz classic-ESP32 override makes the bootloader packaging step look for a `bootloader_qio_40m.elf` that doesn't ship for S3) |
| C3 | `esp32-c3-devkitm-1` | `esp32c3` | `toolchain-riscv32-esp@12.2.0+20230208` | `-march=rv32imc_zicsr_zifencei` (without it: assembler error `csrr ... extension zicsr required` — GCC 12.2's RISC-V default ISA baseline doesn't imply Zicsr/Zifencei the way the framework headers assume) |
| S2 | `esp32-s2-saola-1` | `esp32s2` | `toolchain-xtensa-esp32s2@12.2.0+20230208` | none beyond the toolchain pin |

Also needed: `board_build.mcu` must be set explicitly per env. `env:esp32dev_ble`
sets `board_build.mcu = esp32` for the classic build; any env that `extends`
it inherits that override and silently forces the classic-ESP32 toolchain
regardless of `board =`, producing incomprehensible compiler-version-specific
errors instead of an obvious "wrong chip" failure.

### GCC 8.4.0 miscompiles a pimpl pattern (toolchain issue, not a chip issue)

Using each board's *default* toolchain (all ship GCC `8.4.0+2021r2-patch5` by
default in this platform release, same as classic ESP32's pre-pin default)
fails identically on every chip with:

```
error: invalid application of 'sizeof' to incomplete type 'Adafruit_SSD1306'
```

from `Ssd1306Device.h`'s forward-declared `std::unique_ptr<::Adafruit_SSD1306>`
member — a standard pimpl pattern with the destructor defined out-of-line in
the `.cpp`. GCC 8.4.0 instantiates the deleter early regardless; GCC 12.2.0
does not. This is exactly why `env:esp32dev_ble` already pins
`toolchain-xtensa-esp32@12.2.0+20230208` instead of taking the platform
default — the same pin (for the matching per-chip toolchain package) is
required for every other chip too, not optional.

### Fixed: `VSPI`/`HSPI` don't exist outside classic ESP32

`src/devices/bus/spi/ArduinoSpiBusDriver.cpp` mapped `SpiBusConfig::host` to
Arduino's `VSPI`/`HSPI` enum values unconditionally. Those macros are classic-
ESP32-only (two named SPI peripherals); S2/S3/C3/C6 have a single default SPI2/
SPI3 host selected automatically by the Arduino core. Fixed with a
`#if defined(VSPI)` guard: on chips without it, `ArduinoSpiBusDriver::begin()`
constructs `new SPIClass()` with no host argument (the explicit `sckPin`/
`mosiPin`/`misoPin` it already takes fully describe the bus either way) and
lets the Arduino core assign the free peripheral.

### Flash and RAM fit comfortably on every candidate chip

`my_partitions.csv` requires exactly 4 MB of flash total on the four chips
sharing its 3 MB `app0` slot (`app0` + `devdata` 256 KB + `littlefs` 640 KB +
`nvs` 64 KB = `0x400000`); C6's board (`esp32-c6-devkitm-1`) has a 4 MB `app0`
in its own partition table, hence the lower flash percentage in the results
table above despite a similar absolute binary size. See the results table
above for the real, measured numbers — no chip is a memory risk. The earlier
paper comparison that called S2 the "tightest candidate" (least total SRAM of
the WiFi-capable families) did not hold up: its real measured usage is in
line with classic ESP32.

## ESP32-C6: works, but needed Arduino-ESP32 core 3.x

`platformio/espressif32` (the official platform, every version checked up to
`7.0.1`) cannot build C6 with Arduino at all: it only ships
`framework-arduinoespressif32 @ 3.20017.241212+sha.dcc1105b` (PlatformIO's
version numbering for Arduino-ESP32 core **2.0.17**), and C6 Arduino support
was only ever added upstream in core **3.x**. Every C6 board manifest in the
official platform lists `frameworks: ["espidf"]` only; `framework = arduino`
fails immediately with `Error: This board doesn't support arduino
framework!`, before any package download — not a version-pinning problem, no
released official-platform version can do this (confirmed by checking
`6.9.0` through `7.0.1`, and by the platform's own changelog, which only adds
C6 *board* entries, never Arduino framework support for them).

`env:esp32c6` in `platformio.ini` instead uses the community
**[pioarduino/platform-espressif32](https://github.com/pioarduino/platform-espressif32)**
fork (`platform = https://github.com/pioarduino/platform-espressif32/releases/download/54.03.20/platform-espressif32.zip`),
which repackages the real Espressif Arduino-ESP32 3.2.0 release (confirmed
via [PlatformIO community: "Confused in 2025: does PlatformIO support
ESP32-C6 for Arduino code?"](https://community.platformio.org/t/confused-in-2025-does-platform-io-support-esp32-c6-for-arduino-code/47300)).
This is a different core generation than classic/S2/S3/C3 (which stay on the
official platform's core 2.x) — `env:esp32c6` is the only environment on
core 3.x/`pioarduino`, isolated from the rest.

### Core 2.x → 3.x API breaks found and fixed (all in `src/`, all small)

Getting the project's own source to compile against core 3.x surfaced a
short, finite list of renamed/removed APIs — not a wide rewrite. Each is
guarded with `#if ESP_ARDUINO_VERSION_MAJOR >= 3` (from `<esp_arduino_version.h>`,
included transitively by `<Arduino.h>`) so classic/S2/S3/C3 keep using the
core 2.x calls unchanged:

| File | Core 2.x | Core 3.x | Note |
|---|---|---|---|
| `ArduinoAdcInputDriver.cpp` | `adcAttachPin(pin)` | *(nothing — implicit on first read)* | ADC channel attachment is automatic now |
| `LedcAnalogOutputDevice.cpp` | `ledcSetup(ch, freq, bits)` + `ledcAttachPin(pin, ch)` | `ledcAttachChannel(pin, freq, bits, ch)` | explicit channel still supported, just one call |
| `LedcAnalogOutputDevice.cpp` | `ledcWrite(ch, duty)` | `ledcWriteChannel(ch, duty)` | rename |
| `LedcAnalogOutputDevice.cpp` | `ledcDetachPin(pin)` | `ledcDetach(pin)` | rename |
| `RmtPulseCapture.h`/`.cpp` | `rmt_obj_t* handle` + `rmtInit(pin, mode, memsize)` returning a handle, `rmtSetTick`, `rmtSetRxThreshold(handle, ...)`, `rmtReadAsync(handle, items, count, cb, wait, timeout)`, `rmtReceiveCompleted(handle)`, `rmtEnd`/`rmtDeinit(handle)` | pin-addressed, no handle: `rmtInit(pin, dir, memsize, freq_Hz)` returns `bool`, `rmtSetRxMaxThreshold(pin, ...)`, `rmtReadAsync(pin, items, &count)`, `rmtReceiveCompleted(pin)`, `rmtDeinit(pin)` | full API restructure (used by the DHT11 driver's RMT receive backend); no `rmtEnd` equivalent — `cancel()` just clears `pending_`, `release()` is the one that calls `rmtDeinit` |
| `ArduinoNtpClient.cpp` | `udp_.flush()` (drained rx as a side effect) | `udp_.clear()` (`flush()` still exists but now only means tx-flush, `-Werror=deprecated-declarations` catches the old call) | matches `flushPackets()`'s actual intent (drain stale rx) |
| `RandomBlobKey.cpp` | `esp_random()` pulled in transitively via `<esp_system.h>` | needs its own `#include <esp_random.h>` | header no longer re-exports it |

### Third-party library gap found and worked around: `OneWire` doesn't know about C6

`paulstoffregen/OneWire`'s registry release (2.3.8, the latest tagged
release, same one every other env uses) fails to compile for C6:
`OneWire_direct_gpio.h`'s ESP32 branch only special-cases
`CONFIG_IDF_TARGET_ESP32C3` to use the chip's typed GPIO register structs
(`GPIO.out_w1tc.val = ...`); C6 needs the identical treatment but falls
through to the "plain ESP32" branch, which assumes a raw `uint32_t` register
and fails to compile against C6's typed struct. This is exactly the kind of
one-off, direct-register bit-banging (`OneWire::reset()`/`write_bit()`/
`read_bit()`, timing-critical, called with interrupts disabled) that trades
portability for speed, so it isn't chip-agnostic by design.

Fixed upstream — `PaulStoffregen/OneWire`'s `master` branch (verified via
GitHub's compare API to be exactly 2 commits/1 file ahead of the `v2.3.8` tag,
no unrelated changes bundled in for the C6 fix itself) adds
`|| CONFIG_IDF_TARGET_ESP32C6` to the same 5 functions already special-cased
for C3 — but there is no tagged release with the fix yet. `env:esp32c6`
points `lib_deps` at `https://github.com/PaulStoffregen/OneWire.git#master`
for this one library, in this one env only; every other env keeps the
registry release. Re-pin to a tagged release once `2.3.9+` ships it.

Note: `master` also carries one unrelated change (a loosened `pin <= 33`
output-pin check on classic ESP32's branch, PR #146) that this project does
not use or need — it only affects classic ESP32, which stays on the registry
release, so it has no effect here. A separate, still-open, not-yet-merged PR
(#160) adds ESP32-H2 support the same way; irrelevant since H2 has no WiFi.

## 1. Classic ESP32 (Xtensa LX6, dual-core) — compiles today

### ESP32 DevKit V1 (DOIT, 30-pin)

The most common cheap/generic "ESP32 dev board" — what most tutorials and the
HiLetgo ESP-WROOM-32 module are built on.

- Exposed GPIO: 0, 1, 2, 3, 4, 5, 12, 13, 14, 15, 16, 17, 18, 19, 21, 22, 23,
  25, 26, 27, 32, 33, 34, 35, 36, 39 (25 usable pins total)
- Input-only: 34, 35, 36, 39
- Strapping: 0, 2, 5, 12, 15
- ADC1: 32, 33, 34, 35, 36, 39
- ADC2: 0, 2, 4, 12, 13, 14, 15, 25, 26, 27
- DAC (true analog out): 25, 26
- I2C default: SDA=21, SCL=22
- Note: does not break out GPIO6-11 (internal flash) at all — no risk of a
  user wiring to them on this board.

### NodeMCU-32S / generic ESP-WROOM-32 (38-pin)

Wider header than the 30-pin DevKit V1; same chip/module.

- Exposed GPIO: same core set as DevKit V1, plus GPIO6-11 are physically
  broken out on the header (labeled SCK/CMD/D0-D3) — wired to the module's
  internal SPI flash, must not be reused despite being present on the header.
- Input-only: 34, 35, 36, 39
- Strapping: 0, 2, 5, 12, 15
- ADC1: 32, 33, 34, 35, 36, 39
- ADC2: 0, 2, 4, 12, 13, 14, 15, 25, 26, 27
- I2C default: SDA=21, SCL=22

### Wemos/LOLIN32 and LOLIN32 Lite

Smaller board with onboard 18650 battery connector + charger (TP4054).

- LOLIN32: 24 usable GPIO pins, same classic-ESP32 pin facts as above (16
  ADC-capable, 2 DAC on 25/26, I2C SDA=21/SCL=22, TX/RX=1/3).
- LOLIN32 Lite: 23 usable GPIO pins (26-pin header), same pin facts; one
  status LED wired to GPIO22 (shared with I2C SCL — a fixed-purpose
  conflict to flag if I2C is also used).

### ESP32-CAM (AI-Thinker)

Very common low-cost board, but most GPIOs are consumed internally by the
camera interface and (optional) microSD card — only a handful remain free.

- Free/usable GPIO: 0, 1, 3, 4, 12, 13, 14, 15, 16 (only ~10 pins total,
  several shared with camera/SD depending on whether SD is used)
- GPIO1/GPIO3 double as the only UART (no onboard USB-serial chip — an
  external FTDI adapter is required to flash/program it)
- Onboard red LED fixed on GPIO33 (active-low)
- No ADC2 available (camera peripheral occupies it)
- No dedicated reset/boot buttons — must jumper GPIO0 to GND to enter flash
  mode manually

### Wemos/LOLIN D1 Mini ESP32

Same tiny D1-Mini form factor as the popular ESP8266 board, ESP32 chip
inside; CH340C USB-serial onboard.

- 26 usable GPIO exposed (2.54mm pitch)
- VSPI on the usual pins: MOSI=23, MISO=19, SCK=18, CS=5
- I2C default: SDA=21, SCL=22
- U0 UART: TX=1, RX=3 (CH340C)
- Otherwise same classic-ESP32 facts (input-only 34-39, strapping 0/2/5/12/15,
  flash-reserved 6-11 not broken out)

### Adafruit HUZZAH32 Feather

Adafruit's ESP32 Feather-format board; common in the Adafruit/CircuitPython
ecosystem, has a LiPo battery connector + charger like the LOLIN32.

- I2C fixed on GPIO22 (SCL) / GPIO23 (SDA) — note this is the **reverse** of
  the SDA=21/SCL=22 convention most other classic-ESP32 boards use, a
  board-specific gotcha
- GPIO0 wired to an onboard boot-mode tactile button
- GPIO35 wired internally to a battery-voltage divider (read-only, do not
  drive as output)
- GPIO12 has a built-in pull-down — recommended output-only, avoid letting
  it float during boot
- ADC2 pins (12, 14, 15, 27, 32-shared) unusable once WiFi is running, same
  caveat as every other classic-ESP32 board

### TTGO/LilyGO T-Display (ESP32, built-in 1.14" ST7789 LCD)

Very popular "has a screen out of the box" board for status displays.

- Only 16 usable GPIO exposed (24-pin header) — 6 GPIO are permanently
  dedicated to the built-in display and unavailable for anything else:
  TFT_MOSI=19, TFT_SCLK=18, TFT_CS=5, TFT_DC=16, TFT_RST=23, TFT_BL=4
- Two onboard buttons fixed on GPIO0 and GPIO35 (input-only pin, so
  button-only use, cannot drive it)
- I2C default: SDA=21, SCL=22
- 13 ADC-capable pins remain among the 16 usable GPIO

### Heltec WiFi Kit 32

Built-in SSD1306 OLED (I2C), popular for the same "has a screen" niche as
the T-Display, LoRa variants share the same base board.

- OLED occupies I2C pins GPIO4 (SDA) and GPIO15 (SCL) — **not** the usual
  21/22 convention, plus GPIO16 for the display's hardware reset line; all
  three are unavailable for anything else
- Otherwise same classic-ESP32 facts apply to the remaining exposed GPIO

### WT32-ETH01 (Ethernet module, no WiFi antenna needed)

Popular when wired Ethernet is wanted instead of/alongside WiFi; far fewer
usable pins than a typical dev board because the LAN8720A PHY consumes most
of them internally.

- Only 15 GPIO broken out (28-pin header) — much less than the ~24 typical
  classic-ESP32 board
- GPIO0, GPIO2, GPIO15 are boot-strapping/mode pins here too, but also
  double as Ethernet-link/status LEDs (GPIO2 = green link LED, GPIO15 = red
  power LED) — effectively unavailable for general GPIO use
- I2C default pins (21/22) are present but share PCB traces with the
  factory-programmed MAC-address EEPROM — reusing them risks corrupting
  calibration data
- No camera/display, just the ethernet PHY + a handful of free GPIO for
  relays/sensors

## 2. ESP32-S3 (Xtensa LX7, dual-core, native USB) — future compile target

Needs: new `platformio.ini` environment (`board_build.mcu = esp32s3`,
`toolchain-xtensa-esp32s3` instead of the currently-pinned
`toolchain-xtensa-esp32`) and its own pin dataset.

### ESP32-S3-DevKitC-1

Espressif's official S3 dev board.

- 36 usable GPIO across two headers (J1/J3), 44 pins total incl. power/GND
- Input-only: GPIO46
- Reserved (internal flash/PSRAM on Octal-SPI module variants): GPIO26-32 —
  must not be reused on modules with Octal PSRAM; still free on Quad-SPI/
  no-PSRAM variants, so this is a per-module fact, not a fixed board fact
- Safe general-purpose pool commonly cited: 1, 2, 4-10, 11-18, 19-21, 33-34,
  38-42, 43-44, 47-48
- ADC1: GPIO1-10; ADC2: GPIO11-20 (ADC2 shares the WiFi-conflict caveat like
  classic ESP32)
- Native USB (GPIO19/20) usable as a second serial/JTAG interface

### Seeed Studio XIAO ESP32-S3 (Sense)

Thumb-sized (21×17.5mm) board, very popular for compact/wearable projects;
the "Sense" variant adds an onboard camera + microphone.

- Only 11 usable GPIO exposed (14-pin header, 2.54mm pitch) — far fewer than
  the DevKitC-1 despite the same chip, because of the small form factor
- 9 of those 11 pins are ADC-capable
- 1x UART, 1x I2C, 1x SPI, 1x I2S share the same 11 pins (multiplexed, not
  separate dedicated pins)
- Onboard user LED, charge LED, reset button, boot button
- Sense variant: 2 additional GPIO reserved for the camera/mic, not
  available for general use

### Freenove ESP32-S3-WROOM

Generic S3 dev board with an onboard microSD slot, popular as an
affordable S3 breakout.

- GPIO35-37 unavailable only if the module's OPI PSRAM is in use (freed up
  on non-PSRAM/Quad-PSRAM variants) — a per-module fact, not fixed
- GPIO38-40 permanently dedicated to the onboard microSD card
- Otherwise standard S3 GPIO/ADC facts apply to the rest

### LilyGO T-Display-S3 (built-in 1.9" LCD)

The S3 counterpart to the classic-ESP32 T-Display; same "has a screen"
niche, now with touch on some variants.

- Only 13 usable GPIO exposed (20-pin header) — most of the chip's GPIO are
  consumed by the onboard display interface
- GPIO15 is a peripheral-power-enable pin that must be driven HIGH at boot
  or the LCD/other onboard peripherals stay unpowered — effectively a
  mandatory fixed-purpose pin, not general-purpose despite being exposed

## 3. ESP32-C3 (RISC-V, single-core) — future compile target

Needs: new `platformio.ini` environment (`board_build.mcu = esp32c3`, a
RISC-V toolchain instead of the currently-pinned Xtensa one) and its own pin
dataset — ADC/strapping facts are substantially different from classic ESP32.

### ESP32-C3-DevKitM-1

Espressif's official C3 dev board; built-in USB/JTAG debugging (no external
debug probe needed).

- Almost all chip GPIOs broken out across two 15-pin headers (J1/J3)
- Commonly cited "safe" pins: 0, 1, 3, 4, 6, 7, 10
- Strapping: 2, 8, 9
- ADC1: GPIO0-4 (5 channels); ADC2: 1 channel
- All GPIOs are 3.3V only, not 5V-tolerant

### ESP32-C3 Super Mini

Extremely cheap (~$2), tiny generic C3 board; very popular for compact/
wearable projects, widely sold under this exact name.

- 13 GPIO exposed across 16 pins (2.54mm pitch): GPIO0-10, 20, 21
- GPIO0-5 are ADC-capable
- Strapping: 2, 8, 9
- Onboard blue status LED fixed on GPIO8
- Onboard BOOT button fixed on GPIO9
- GPIO1=UART TX, GPIO3=UART RX (also usable as general GPIO if not flashing)

### Seeed Studio XIAO ESP32-C3

Same thumb-sized form factor as the XIAO S3, RISC-V chip instead.

- Only 11 GPIO exposed (14-pin header) — even fewer usable pins than the
  DevKitM-1 for the same reason as the S3 XIAO (compact form factor)
- Only 3 of those are genuinely usable as ADC (A0/A1/A2, i.e. ADC1 channels)
  — the WiFi/BLE-shared ADC2 pin (GPIO5) is present on the header but
  unreliable for analog reads while the radio is active, same caveat as
  classic ESP32's ADC2
- 44µA deep-sleep current — the reason it's popular for battery projects

## 4. ESP32-S2 (Xtensa LX7, single-core, native USB, no BLE) — future compile target

No Bluetooth/BLE radio at all (WiFi-only chip) — a hard architectural
difference from every other family here, not just a pin-table difference.
This project's BLE provisioning path (`WITH_BLE_PROVISIONING`) could never
run on S2; only the non-BLE `env:esp32dev`-equivalent flow would apply.

### ESP32-S2-Saola-1

Espressif's official S2 dev board (WROVER module).

- Strapping: 0, 45, 46
- Input-only: GPIO46 (also fixed pull-down)
- Reserved (internal flash/PSRAM): GPIO26-32
- JTAG (usable as GPIO if not debugging): GPIO39-42
- ADC1 default-enabled channels: GPIO0-3 (channels 0-3), more available but
  not enabled by default
- Native USB present (no separate USB-serial chip needed on later revisions)

### Adafruit QT Py ESP32-S2

Tiny (Seeed XIAO-compatible footprint) board, popular for compact WiFi
projects; STEMMA QT connector for I2C (separate from the general GPIO,
not sharing pins with them).

- 13 GPIO exposed total
- Onboard RGB NeoPixel (with a software-controlled power-enable pin) and a
  button fixed on GPIO0
- DAC output available (S2 has 2 true DAC channels, same concept as classic
  ESP32's GPIO25/26)

## 5. ESP32-C6 (RISC-V, single-core, WiFi 6 + Thread/Zigbee + BLE) — future compile target

Newest mainstream family, actively displacing C3 in new designs because it
adds an 802.15.4 radio (Thread/Zigbee/Matter) alongside WiFi 6 + BLE 5 — same
pin-table problem as C3, different specific numbers.

### Espressif ESP32-C6-DevKitC-1

Official C6 dev board; two USB-C ports (one plain USB-UART, one native
USB-Serial/JTAG straight off the chip).

- 23 GPIO exposed across 32 pins
- Strapping: GPIO4, GPIO5, GPIO8, GPIO9, GPIO15
- Native-USB-only pins: GPIO12 (D-), GPIO13 (D+) — don't repurpose if the
  native USB port is used
- Onboard addressable RGB LED (WS2812) on GPIO8 — shared with a strapping
  pin, a fixed-purpose conflict to flag
- ADC1: 8 channels total, GPIO0-6 the commonly-recommended subset

### ESP32-C6 Super Mini

Cheapest (~$5) way to get Zigbee 3.0/Thread/Matter-over-Thread; same
"Super Mini" niche as the C3 version.

- 20 GPIO exposed across 25 pins (2.54mm pitch)
- Same strapping/reserved facts as the DevKitC-1 (same chip)

### Seeed Studio XIAO ESP32-C6

Thumb-sized form factor, positioned by Seeed specifically as Matter-friendly.

- Only 11 GPIO exposed (14-pin header) — same reduced-pin-count pattern as
  every other XIAO board regardless of chip

## 6. ESP32-H2 (RISC-V, single-core, Thread/Zigbee/BLE — **no WiFi**) — not usable for this project

Only family here with **no WiFi radio at all** — BLE 5 + 802.15.4
(Thread/Zigbee/Matter) only. Since this project's entire portal/SPA
architecture is a WiFi-hosted web server (`PortalServer`, `WifiManager`),
an H2 board cannot run this firmware's core function regardless of any pin
compatibility work — listed here only for completeness of the chip-family
research, not as a real future compile target.

### Espressif ESP32-H2-DevKitM-1

- 19 usable GPIO: 0-5, 8-14, 22-27
- GPIO13/14 unavailable if wired to an onboard 32.768kHz crystal (board
  revision dependent)
- Strapping and other facts differ from every other family; not detailed
  further since the no-WiFi blocker makes this moot for this project

## Sources

- ESPHome board registry: `esphome/components/esp32/boards.py` (github.com/esphome/esphome)
- Tasmota supported-device templates: templates.blakadder.com, tasmota.github.io/docs/ESP32
- Vendor/community pinout references: espboards.dev, lastminuteengineers.com,
  mischianti.org, randomnerdtutorials.com, esp32.co.uk, wiki.seeedstudio.com
  (per-board pages linked above by board name)
