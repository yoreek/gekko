## Why

The current `DummyDevice` owns too many responsibilities at once: generic runtime lifecycle, parent/child runtime wiring, retained output state, command handling, and simulated output behavior. New hardware-backed device types need a shared base structure so each new type implements only its hardware-specific behavior.

Switch-like devices are the next device family and need a reusable model for on/off state, inversion, startup behavior, retained state, and command handling before adding concrete GPIO or I2C expander implementations.

## What Changes

- Introduce a reusable base runtime class for common dynamic device behavior currently duplicated or embedded in `DummyDevice`.
- Refactor `DummyDevice` to inherit from the base runtime while preserving its public descriptor, config compatibility, lifecycle behavior, retained state support, and tests.
- Introduce a switch-device base class for binary output devices with on/off state, command handling, inversion, startup state policy, and retained-state integration.
- Add a concrete GPIO switch device type that owns GPIO pin configuration and delegates common switch behavior to the switch base.
- Keep device runtime work cooperative and cadence-driven through the existing registry tick flow.
- Keep persistent type configuration binary and versioned; current output state remains retained runtime state, not repeated config rewrites.
- No new external dependencies are required.

## Capabilities

### New Capabilities

- `device-runtime-hierarchy`: Defines reusable base runtime behavior for dynamic devices and the required inheritance structure for `DummyDevice`.
- `switch-device-runtime`: Defines the shared switch-device model and the first concrete GPIO switch runtime.

### Modified Capabilities

- `device-registry`: Registry runtime creation and retained-state expectations must support inherited runtime classes and switch-like retained output state.
- `device-type-catalog`: The supported type catalog must include the new GPIO switch device type with a stable numeric `type_id`.

## Impact

- Firmware code under `src/devices/core/`, `src/devices/dummy/`, and new switch/GPIO device folders.
- Device type registration in `DeviceTypeRegistry::withDefaults()`.
- Device config binary codecs and tests for `DummyDevice`, base switch config, and GPIO switch config.
- Device registry tests for runtime creation, retained state, commands, and lifecycle transitions.
- Portal/frontend type catalog and localization may need an additional switch type label when the GPIO switch type becomes user-visible.
