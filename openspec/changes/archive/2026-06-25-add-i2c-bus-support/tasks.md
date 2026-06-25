## 1. Core I2C Model

- [x] 1.1 Add an `devices/bus/i2c/` module with I2C address/config helpers that mirror the existing OneWire bus layout.
- [x] 1.2 Add the I2C dependency role and runtime address hook needed for dependent devices to identify themselves by raw 7-bit address.
- [x] 1.3 Define the I2C bus config contract with SDA, SCL, and bus clock frequency fields plus JSON/binary encoding helpers.

## 2. I2C Bus Runtime

- [x] 2.1 Implement an Arduino `TwoWire` bus driver wrapper that owns bus initialization, shutdown, clock updates, and transaction access.
- [x] 2.2 Implement the `I2cBusDevice` runtime state machine with explicit `Idle`/`Starting`/`Ready`/`Reconfiguring` lifecycle handling.
- [x] 2.3 Add bus reconfiguration handling so changes to SDA, SCL, or frequency release the old hardware and restart the runtime cleanly.
- [x] 2.4 Add shared transaction guarding so only one dependent device can use the bus at a time and generation changes invalidate stale access.

## 3. Registry and API Wiring

- [x] 3.1 Register the new I2C bus device type in the device type catalog and dependency compatibility tables.
- [x] 3.2 Add REST API support for creating, updating, and serializing the I2C bus device configuration and runtime state.
- [x] 3.3 Add device registry and setup transfer codec support for the new I2C bus device config blob.
- [x] 3.4 Ensure duplicate raw I2C addresses are rejected when dependents are attached to the same bus.

## 4. Verification

- [x] 4.1 Add focused tests for I2C bus config parsing, validation, and reconfiguration behavior.
- [x] 4.2 Add focused tests for shared transaction guarding and duplicate-address rejection.
- [x] 4.3 Run `scripts/test.sh` and fix any formatting, static analysis, or native test failures.
