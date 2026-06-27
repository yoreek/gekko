## Why

The firmware needs a shared SPI bus foundation before adding ST7735 or other SPI-backed devices. SPI devices share the same clock and data lines, but each peripheral is selected by its own chip-select pin, so the backend needs a bus-level runtime that owns the common wiring and leaves per-device selection to future device types.

## What Changes

- Add a dedicated SPI bus runtime that follows the existing bus-device pattern used by I2C and OneWire.
- Model shared SPI wiring at the bus level: controller/host selection plus common `SCK`, `MOSI`, and optional `MISO`.
- Keep chip-select ownership at the dependent device level so multiple peripherals can share one SPI bus.
- Add runtime lifecycle handling for initialization, release, restart, and guarded shared access for dependent SPI devices.
- Add dependency validation and duplicate chip-select protection so future SPI peripherals can attach safely to the same bus.
- Add REST, registry, setup-transfer, and device-type wiring for the new SPI bus device.

## Capabilities

### New Capabilities
- `spi-bus-device`: shared SPI bus runtime with host selection, common wiring, lifecycle management, and dependency coordination for future SPI peripherals.

### Modified Capabilities
- None.

## Impact

Affected areas include `src/devices/bus/`, device type registration, dependency-role plumbing, REST adapters, registry/setup codecs, and tests. This change is backend-only and prepares the SPI foundation for later ST7735 device support without adding display rendering yet.
