## Context

The firmware already has bus-owned runtime patterns for OneWire and I2C. Those buses own shared hardware setup, expose guarded access for dependents, and restart cleanly when hardware settings change. SPI needs the same architectural treatment before any SPI-backed device such as ST7735 is added.

On ESP32, SPI is a shared bus: `SCK`, `MOSI`, and `MISO` are common lines, while `CS` is selected per device. A bus runtime therefore needs to own the shared controller and wiring, but it should not try to encode device selection as an address model like I2C. The implementation also needs to stay within the existing firmware model: cooperative lifecycle, explicit state transitions, bounded memory, and API/registry wiring consistent with the current bus devices.

## Goals / Non-Goals

**Goals:**
- Add a dedicated SPI bus runtime that owns shared SPI hardware setup and lifecycle.
- Support shared bus wiring with `host`, `SCK`, `MOSI`, and optional `MISO`.
- Keep `CS` ownership on future SPI peripheral devices rather than on the bus.
- Expose guarded bus access so future SPI devices can coordinate transfers safely.
- Add dependency-role and registry/API plumbing so the backend can register SPI bus devices now and future SPI peripherals later.

**Non-Goals:**
- Add ST7735 rendering or any other SPI peripheral implementation in this change.
- Add frontend UI changes.
- Add an address-based SPI dependency model; SPI devices are selected by `CS`, not by address.
- Add support for multiple simultaneous SPI bus controllers beyond the ESP32 host selection the firmware already needs.

## Decisions

### 1. Model SPI as a dedicated bus runtime, not a generic transport layer
The first version should mirror `OneWireBusDevice` and `I2cBusDevice`: a concrete runtime with its own config, validation, dependency wiring, and lifecycle transitions. That keeps the firmware model consistent and avoids introducing a generic bus abstraction before there are multiple SPI-class peripherals using it.

Alternative considered: build a shared transport framework for all buses. Rejected for now because it adds abstraction cost without solving an immediate product need.

### 2. Own the shared SPI controller inside the bus runtime
The runtime should own its `SPIClass` instance and manage `begin()`/`end()` explicitly rather than relying on a global singleton. That makes bus startup and reconfiguration deterministic and keeps the lifetime of the shared controller visible in the runtime state machine.

Alternative considered: use a global `SPI` object directly. Rejected because it obscures ownership, makes restart behavior less explicit, and complicates future test coverage.

### 3. Keep bus config limited to shared wiring and controller selection
The SPI bus config should cover the shared physical bus: controller host selection plus `SCK`, `MOSI`, and optional `MISO`. `CS` must stay out of bus config because it belongs to each peripheral device, and device-specific SPI settings such as clock, mode, and bit order should be applied by future peripheral runtimes during guarded transactions.

Alternative considered: include `CS` or per-device mode/clock in the bus config. Rejected because those values are owned by individual peripherals and should not be coupled to the shared bus topology.

### 4. Allow optional `MISO` on the bus
Some SPI deployments are write-only, while others need bidirectional transfers. The bus should accept a shared `MISO` line when hardware uses it, but it should not require it for every bus instance. That keeps the bus usable for display-oriented devices while still supporting read-capable peripherals later.

Alternative considered: require `MISO` for all SPI buses. Rejected because many common display setups do not use it and the firmware should not force unnecessary wiring.

### 5. Limit host selection to the ESP32 general-purpose controllers
The runtime should expose the controllers that are safe and useful for user devices on ESP32, namely the general-purpose SPI hosts. The flash/boot controller should remain out of scope because it is not a normal user bus and has different hardware constraints.

Alternative considered: expose every hardware SPI controller. Rejected because it would invite invalid configurations around the flash bus and make the contract less clear.

### 6. Use a transaction guard for shared SPI access
The bus should expose a guarded transaction object similar to the existing bus patterns. Future SPI peripherals can acquire the bus, apply their own `clockHz`, `mode`, and `bitOrder`, assert their own `CS`, and then release the bus. This keeps mutual exclusion and bus reconfiguration rules in one place.

Alternative considered: let peripherals call the driver directly. Rejected because it would leave bus sharing, reconfiguration, and stale-handle protection up to each device.

### 7. Treat `CS` uniqueness as a device-level validation rule
SPI bus validation should be able to detect duplicate `CS` pins among dependent devices attached to the same bus, but the bus itself should not own those pins. This matches the physical bus model and keeps future ST7735-style devices self-contained.

Alternative considered: model `CS` as part of bus config. Rejected because `CS` is a per-device selection line, not a property of the shared bus.

## Risks / Trade-offs

- [Risk] The first SPI bus version will not solve per-device mode negotiation by itself. → Mitigation: include a transaction guard and leave per-device SPI settings to future peripheral runtimes.
- [Risk] Optional `MISO` means some bus instances will be write-only. → Mitigation: make that explicit in validation and keep the runtime focused on the shared wiring actually configured.
- [Risk] Limiting controller selection to the general-purpose SPI hosts excludes flash-bus reuse. → Mitigation: keep that scope narrow on purpose; user peripherals should not depend on the flash controller.
- [Risk] `CS` validation depends on future peripheral runtimes implementing the relevant identity hook. → Mitigation: add the runtime hook now and cover it with tests when the first SPI peripheral lands.

## Migration Plan

1. Add the SPI bus device type, config, driver abstraction, and runtime alongside the existing bus code.
2. Register the new type in the device type catalog, REST adapters, and setup/registry codecs.
3. Add dependency-role plumbing and bus transaction support for future SPI peripherals.
4. Keep existing I2C and OneWire behavior unchanged.
5. If the change needs to be rolled back, remove the new SPI type registration and related API wiring; no existing device data should depend on it yet.

## Open Questions

- The first version will keep per-device SPI settings on future peripherals rather than in the bus contract.
- `CS` uniqueness will be enforced by future SPI peripheral device validation rather than by the bus config itself.
