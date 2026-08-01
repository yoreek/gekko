# ESP32 Board Models — Research Database

Research only, not wired into any code. Organized by **chip variant**, because
that is what actually determines a compile target: the firmware toolchain and
architecture (Xtensa vs RISC-V) is fixed per chip, not per physical board.
Every board listed under the same chip already runs (or would run) the exact
same compiled binary — they only differ in which GPIOs are physically broken
out and what's permanently wired to what. List gathered from vendor pinout
references, the ESPHome board registry
(`esphome/components/esp32/boards.py`), the Tasmota supported-device/template
repository (templates.blakadder.com), and espboards.dev's board catalog (272
boards across 6 chip families at the time of writing — this doc picks a
handful of genuinely distinct/popular boards per family, not an exhaustive
catalog).

Today this project only builds for classic ESP32 (`platformio.ini`'s
`esp32dev`/`esp32dev_ble`/`esp32dev_ota` environments, `toolchain-xtensa-esp32`
pinned). S3, C3, S2 and C6 have no code-level blocker found in `src/` (no
Xtensa-assembly, no architecture-specific BLE/RTOS calls) but need their own
PlatformIO environment + toolchain + pin dataset before they can actually be
compiled — tracked as future work, not done yet. **H2 has no WiFi radio at
all** and cannot run this project's portal architecture regardless of pin
work — included below only for completeness, not as a real target.

## Memory: will the firmware actually fit?

### Flash — fits comfortably on every board researched here

`my_partitions.csv` requires exactly 4 MB of flash total: `app0` (3 MB) +
`devdata` (256 KB) + `littlefs` (640 KB) + `nvs` (64 KB) = `0x400000`
(measured, not estimated — this is the literal sum of the partition table's
offsets/sizes). The compiled firmware itself currently uses 1.84 MB of the
3 MB `app0` slot (58.5%, from an actual `pio run -e esp32dev` build).

4 MB is the minimum flash size on virtually every board listed above —
several (larger S3 boards especially) ship with 8 or 16 MB, leaving
comfortable headroom. No board researched here reports less than 4 MB, so
flash is not expected to be a blocker for any of the 5 real candidate chip
families (classic, S3, C3, S2, C6).

### RAM — likely fine for most families, ESP32-S2 is the one genuine risk

Current static RAM usage on classic ESP32 is modest: 58,528 / 327,680 bytes
(17.9%), from the same real build. This number is **link-time static data
only** — it does not include runtime heap (AsyncWebServer, WebSocket
buffers, JSON parsing, the device registry, etc. all allocate from the heap
dynamically), so it understates real usage, but it's the only concrete data
point available without actually compiling for each chip.

| Chip | Total SRAM | Assessment |
|---|---|---|
| Classic ESP32 | 520 KB | Proven — this is what's actually running today |
| ESP32-S3 | 512 KB (often + PSRAM) | Likely fine — comparable or more headroom |
| ESP32-C6 | 512 KB HP + 16 KB LP | Likely fine — comparable headroom |
| ESP32-C3 | 400 KB | Probably fine given today's low static usage, unverified |
| **ESP32-S2** | **320 KB, single-core** | **Tightest candidate** — least total SRAM of the WiFi-capable families, and the WiFi/LWIP stack still needs meaningful RAM even without a second core to spread work across |

None of this is verified by an actual build — it's paper comparison of
today's classic-ESP32 numbers against each chip's datasheet SRAM figure.
The only way to know for certain is to add each chip's PlatformIO
environment and read the linker's real RAM/flash report, the same way
`pio run -e esp32dev` did above.

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
