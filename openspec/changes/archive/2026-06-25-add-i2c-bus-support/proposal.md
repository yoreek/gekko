## Why

The firmware needs a shared I2C bus abstraction so upcoming display and temperature sensor devices can reuse one `TwoWire` instance instead of owning ad hoc pin and clock setup. I2C devices share the same bus by address, so the bus layer must own initialization, lifetime, and coordinated access before device-specific drivers are added.

## What Changes

- Add an I2C bus runtime modeled after the existing `OneWireBusDevice` pattern.
- Configure the bus with SDA, SCL, and a bus clock frequency.
- Optionally enable internal pull-ups for SDA and SCL when the board needs them.
- Initialize the Arduino-ESP32 `TwoWire` master bus with the configured pins and clock.
- Allow the bus clock to be updated through the bus config so the runtime can reinitialize safely when hardware settings change.
- Expose shared bus access for future dependent devices that talk to fixed I2C addresses.
- Add config validation for pin values, clock frequency bounds, and address-oriented dependency wiring expected by future device runtimes.
- Add REST, setup, and registry plumbing consistent with the existing bus-device model.

## Capabilities

### New Capabilities
- `i2c-bus-device`: shared I2C master bus runtime with configurable SDA/SCL pins, clock frequency, and dependency coordination for multiple addressed devices on the same bus.

### Modified Capabilities

- None.

## Impact

Affected areas include `src/devices/bus/`, device type registration, REST adapters, setup bundle handling, and dependency role plumbing. The implementation will depend on Arduino-ESP32 `TwoWire` behavior for `begin(sda, scl, frequency)` and `setClock(frequency)`, and it will establish the foundation for later I2C-backed display and sensor devices.
