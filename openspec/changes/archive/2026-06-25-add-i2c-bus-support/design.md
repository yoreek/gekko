## Context

The firmware already has a working OneWire bus pattern: a bus runtime owns hardware setup, exposes a transaction guard, participates in dependency wiring, and reinitializes cleanly when bus configuration changes. I2C needs the same lifecycle discipline, but the underlying protocol differs because multiple devices share one physical bus and each device is selected by raw address rather than by a scanned ROM code.

Arduino-ESP32 exposes this through `TwoWire`, where `begin(sda, scl, frequency)` initializes the bus and `setClock(frequency)` updates the clock after initialization. ESP32 I2C hardware also supports multiple devices on one bus, and the ESP-IDF documentation treats the device address as a raw address without the read/write bit. This change is scoped to the shared bus runtime and the foundation for future display and temperature sensor devices.

## Goals / Non-Goals

**Goals:**
- Add a bus runtime that owns `TwoWire` lifecycle explicitly.
- Support configurable SDA, SCL, and bus clock frequency.
- Allow multiple dependent devices to share the bus safely by raw address.
- Keep the implementation aligned with the existing OneWire bus runtime pattern so the rest of the firmware can consume it consistently.

**Non-Goals:**
- Add the display or temperature sensor device implementations themselves.
- Add per-device clock negotiation for individual I2C peripherals in the first iteration.
- Add multi-controller selection or 10-bit address support unless a later requirement needs it.
- Replace the existing OneWire abstractions.

## Decisions

### 1. Model I2C as a dedicated bus runtime instead of a generic transport layer
The new capability should look like `OneWireBusDevice`: a typed runtime with its own config, dependency handling, and lifecycle transitions. This keeps the firmware model consistent and avoids introducing a generic transport abstraction before there is more than one shared bus family in regular use.

Alternative considered: build a generic bus framework and plug OneWire/I2C into it. Rejected for now because it adds indirection without solving an immediate user problem.

### 2. Own a `TwoWire` instance inside the runtime
The runtime should manage its own `TwoWire` object rather than relying on the global `Wire` singleton. That keeps begin/end behavior explicit, makes config reinitialization deterministic, and avoids hidden coupling between unrelated code paths.

Alternative considered: reuse `Wire` globally. Rejected because global ownership would make teardown and future testability less explicit, and it would be harder to reason about multiple bus instances if the project ever needs them.

### 3. Treat frequency as a bus-level setting in the first iteration
The runtime will store a single clock frequency in bus config and apply it when the bus starts or restarts. This matches the Arduino-ESP32 API and keeps the contract simple for all dependents sharing the same physical bus.

Alternative considered: expose per-transaction speed control immediately. Rejected because it would push device-specific policy into the shared bus layer and complicate dependency coordination before there is a concrete need.

### 4. Expose internal pull-up control in the bus config
The bus config should allow enabling internal pull-ups for SDA and SCL. Some boards need them for bring-up or low-risk defaults, and the Arduino/ESP-IDF stack already supports that style of configuration at the bus level.

Alternative considered: omit pull-up control and rely only on external resistors. Rejected because the board-level wiring is not uniform across all deployments, and the runtime should make this choice explicit.

### 5. Represent dependent devices by raw 7-bit address
Dependent I2C devices should identify themselves with a raw 7-bit address, not an address byte with R/W bit baked in. That matches the ESP-IDF documentation and the Arduino I2C API expectations, and it avoids ambiguous identity encoding.

Alternative considered: store the transmitted address byte or support 10-bit addressing from the start. Rejected because the current target devices are standard 7-bit peripherals and widening the contract now would add complexity without immediate value.

### 6. Reuse the existing dependency-transaction pattern
The I2C bus should expose a guarded transaction object similar to `OneWireBusDevice::DependencyTransaction`. The bus runtime already fits a cooperative, state-machine-driven lifecycle, so a transaction guard is the right place to prevent overlapping dependent use and to carry a stable generation token across bus reinitialization.

Alternative considered: let dependents call `TwoWire` directly with no bus-level guard. Rejected because it would make bus reconfiguration and active use harder to coordinate safely.

## Risks / Trade-offs

- [Risk] The first version only supports a single bus frequency for all devices on the bus. → Mitigation: keep the contract explicit and defer per-device clock negotiation until there is a concrete device that needs it.
- [Risk] Depending on `TwoWire` internals may obscure some low-level I2C capabilities. → Mitigation: keep the runtime thin and push protocol-specific behavior into device drivers.
- [Risk] Raw-address-only identity excludes 10-bit devices. → Mitigation: document the limitation and revisit only if a future hardware target requires it.
- [Risk] Reinitializing the bus on config change can briefly disconnect active dependents. → Mitigation: mirror the OneWire generation/restart behavior so dependents can detect bus restarts and rebind cleanly.

## Migration Plan

1. Add the new bus device type, config, and runtime alongside the existing OneWire code.
2. Register the new device type in the registry and REST/setup plumbing.
3. Add dependency role support and shared bus transaction handling for future I2C dependents.
4. Keep OneWire behavior unchanged so existing devices continue to work.
5. If the change needs to be rolled back, remove the new I2C device type registration and related API wiring; existing registry data for the new type can remain unused without affecting OneWire devices.

## Open Questions

- Internal pull-up control is part of the initial config.
- Controller selection will not be exposed in the first release; the runtime will use the default `TwoWire` controller instance.
- The runtime will stay limited to raw 7-bit addressing for now and will not reserve 10-bit address support in the initial API.
