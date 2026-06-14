## 1. Runtime Base Refactor

- [x] 1.1 Add `DeviceRuntimeBase` under `src/devices/core/` with common `IDeviceRuntime` lifecycle request flags, status storage, parent runtime, child runtime, and cadence defaults.
- [x] 1.2 Add focused unit tests for `DeviceRuntimeBase` parent/child wiring, status requests, and cooperative cadence behavior using a small test runtime.
- [x] 1.3 Refactor `DummyDevice` to inherit from `DeviceRuntimeBase` while preserving its descriptor, config payload format, retained state behavior, and command handling.
- [x] 1.4 Extend existing DummyDevice and registry tests to verify lifecycle parity after the refactor.

## 2. Shared Switch Runtime

- [x] 2.1 Add shared switch config structs and binary encode/decode helpers for enabled state, restore-previous-state, startup `OutputState`, safe `OutputState`, and inversion.
- [x] 2.2 Add `OutputState`, `SwitchDeviceBase`, `BinarySwitchDeviceBase`, and `TriStateSwitchDeviceBase` with output-state capabilities, physical state mapping, retained state application, explicit set-state commands, and hardware hook methods.
- [x] 2.3 Add fake switch runtime tests covering startup policy, safe state, inversion, retained state restore, command handling, disabled-output capability, and unsupported command rejection.
- [x] 2.4 Ensure switch output changes integrate with retained-state persistence without incrementing device config revision.

## 3. GPIO Switch Device

- [x] 3.1 Add a bounded `IGpioOutputDriver` interface and a production Arduino GPIO implementation isolated from generic switch logic.
- [x] 3.2 Add `GpioSwitchDevice` config structs, binary codec, JSON adapter helpers, descriptor, and config validation.
- [x] 3.3 Implement `GpioSwitchDevice` runtime hooks for configure, write physical output, disable, reconfigure, and delete behavior.
- [x] 3.4 Add fake GPIO driver tests for valid startup, inversion, explicit set-state commands, disabled output, safe state, invalid pin rejection, and reconfigure.

## 4. Registry And Catalog Integration

- [x] 4.1 Register `GpioSwitchDevice` in `DeviceTypeRegistry::withDefaults()` with stable `type_id = 2`.
- [x] 4.2 Update backend API adapter wiring so create/show/update config flows can parse and serialize GPIO switch config.
- [x] 4.3 Update frontend device type catalog and locale labels for `GpioSwitchDevice`.
- [x] 4.4 Add integration tests for creating, loading, commanding, retaining, and deleting a GPIO switch device through the registry.

## 5. Verification

- [x] 5.1 Run `scripts/test.sh` and fix any firmware or native test regressions.
- [x] 5.2 Run the frontend build if catalog or API contract changes touch `portal-spa`.
- [x] 5.3 Review memory and payload bounds for new switch configs against `kMaxDeviceConfigBytes` and retained-state limits.
