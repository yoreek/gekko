## 1. Contracts And Catalog

- [x] 1.1 Extend the backend device API adapter interface so type-specific adapters receive the persisted `DeviceRecord` and the optional runtime pointer when writing device JSON.
- [x] 1.2 Add GPIO switch runtime `output.state` serialization through `GpioSwitchDeviceApiAdapter` without adding runtime state to `config`.
- [x] 1.3 Align `portal-spa` device API contracts with GPIO switch config, runtime `output.state`, and output-state command payloads.
- [x] 1.4 Extend the device type catalog entry for `GpioSwitchDevice` with label, local icon, component registry key, and supported output states.
- [x] 1.5 Add or update English and Russian labels for GPIO switch config fields, output states, command actions, and validation errors.

## 2. Type-Specific Components

- [x] 2.1 Create shared `models/devices/switch` and `components/devices/switch` modules for `OutputState`, switch config fields, state selects, and output controls reusable by future switch types.
- [x] 2.2 Create a GPIO switch device component folder with decomposed form, detail, and dashboard widget components that reuse the shared switch modules.
- [x] 2.3 Implement the GPIO switch create/edit form using Vuetify inputs, selects, switches, buttons, and standard validation surfaces.
- [x] 2.4 Implement the GPIO switch detail section showing shared fields plus GPIO pin, startup state, safe state, restore-previous-state, inversion, and current output state when present.
- [x] 2.5 Implement explicit output-state controls for `on`, `off`, and `disabled` through the existing device command API.
- [x] 2.6 Implement a compact switch Power dashboard action that uses `output.state`, disables in edit mode, when `effective_status` is not `ready`, or when `output.state` is `disabled`/unknown, and sends explicit `on`/`off` next-state commands only.

## 3. Page Integration

- [x] 3.1 Wire GPIO switch form selection into the existing create-device flow without adding page-level type branches beyond the registry lookup.
- [x] 3.2 Wire GPIO switch details and controls into the Devices page detail dialog.
- [x] 3.3 Wire GPIO switch dashboard widget resolution through the existing device widget registry while preserving compact fixed-size card behavior.
- [x] 3.4 Ensure base dashboard cards show the device name and use `effective_status` only for visual dimming, with no `Ready`, `!Ready`, or raw status text.
- [x] 3.5 Ensure edit mode disables dashboard controls and prevents text/control interaction while cards are being dragged.

## 4. Mock And Verification

- [x] 4.1 Extend mock database and handlers to store GPIO switch config fields and runtime `output.state`.
- [x] 4.2 Extend mock command handling to accept `on`, `off`, and `disabled`, update output state, and reject unsupported states.
- [x] 4.3 Run frontend build/smoke checks and fix type or bundle regressions.
- [x] 4.4 Add or update backend tests for GPIO switch device JSON output serialization.
- [x] 4.5 Browser-check GPIO switch create, detail, command, dashboard widget, and mock-mode flows; verify against ESP32 when available.
