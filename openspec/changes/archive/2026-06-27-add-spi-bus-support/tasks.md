## 1. Core SPI Model

- [x] 1.1 Add a `devices/bus/spi/` module with SPI bus config helpers that mirror the existing bus-device layout.
- [x] 1.2 Add the SPI dependency role plus a runtime identity hook for dependent devices to expose their `CS` pin.
- [x] 1.3 Define the SPI bus config contract with host selection, `SCK`, `MOSI`, and optional `MISO` fields plus JSON/binary encoding helpers.

## 2. SPI Bus Runtime

- [x] 2.1 Implement an Arduino `SPIClass` bus driver wrapper that owns bus initialization, shutdown, and transaction access.
- [x] 2.2 Implement the `SpiBusDevice` runtime state machine with explicit `Idle`/`Starting`/`Ready`/`Reconfiguring` lifecycle handling.
- [x] 2.3 Add bus reconfiguration handling so changes to host, `SCK`, `MOSI`, or `MISO` release the old hardware and restart the runtime cleanly.
- [x] 2.4 Add shared transaction guarding so only one dependent device can use the bus at a time and generation changes invalidate stale access.

## 3. Registry and API Wiring

- [x] 3.1 Register the new SPI bus device type in the device type catalog and dependency compatibility tables.
- [x] 3.2 Add REST API support for creating, updating, and serializing the SPI bus device configuration and runtime state.
- [x] 3.3 Add device registry and setup transfer codec support for the new SPI bus device config blob.
- [x] 3.4 Ensure duplicate `CS` pins are rejected when dependents are attached to the same SPI bus.

## 4. Verification

- [x] 4.1 Add focused tests for SPI bus config parsing, validation, and reconfiguration behavior.
- [x] 4.2 Add focused tests for shared transaction guarding and duplicate-`CS` rejection.
- [x] 4.3 Run `scripts/test.sh` and fix any formatting, static analysis, or native test failures.
