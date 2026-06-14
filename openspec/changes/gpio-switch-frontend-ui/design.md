## Context

`GpioSwitchDevice` is now registered on the firmware side with `type_id = 2`, binary config storage, REST JSON adapter support, and explicit output-state commands. The SPA currently knows the catalog entry and has only a generic dashboard widget path, so users cannot fully create, inspect, or control this device type from the browser.

The SPA must remain Vuetify-first, keep the local icon registry, avoid custom CSS hacks, and preserve the compact dashboard layout rules already defined for device cards.

## Goals / Non-Goals

**Goals:**
- Add GPIO switch type-specific create/edit/detail UI using existing Vuetify form controls.
- Keep type-specific UI decomposed under device component folders and registered through explicit mappings.
- Support explicit output commands for `on`, `off`, and `disabled` through the existing device command API.
- Expose current switch runtime output state from the backend without storing it in device config.
- Update mock mode so the same flows can be tested locally without firmware.
- Preserve dashboard layout behavior and compact card footprint.

**Non-Goals:**
- Do not change backend API endpoints or firmware storage formats.
- Do not add icon packages or broad UI libraries for this device type.
- Do not add automatic polling or websocket behavior in this change.
- Do not put multi-row status/config metadata into dashboard cards.

## Decisions

1. Use explicit frontend type mappings for GPIO switch components.

   The SPA already uses a local device type catalog and device component folders. GPIO switch UI should add a type-specific component folder and registry entries instead of branching throughout page components. This keeps future switch variants independent.

2. Add a shared frontend switch layer before GPIO-specific components.

   GPIO switch is the first concrete switch device, but it already uses concepts that belong to the whole switch family: `OutputState`, startup state, safe state, inversion, restore-previous-state, and explicit set-state commands. The SPA should place reusable switch models and controls under shared `switch` modules, then keep GPIO-specific files focused on `gpio_pin` and GPIO-specific labels.

   Target structure:

   ```text
   portal-spa/src/models/devices/
     base.ts
     switch.ts
     dummy.ts
     gpio-switch.ts

   portal-spa/src/components/devices/
     base/
     switch/
       SwitchConfigFields.vue
       SwitchOutputControls.vue
       SwitchStateSelect.vue
     dummy/
     gpio-switch/
       GpioSwitchDeviceForm.vue
       GpioSwitchDeviceDetail.vue
       GpioSwitchDeviceWidget.vue
   ```

   Future switch implementations such as I2C expander switches should reuse `switch/` models and controls and add only transport-specific fields in their own folders.

3. Model output state as a frontend enum matching backend strings.

   The UI will treat `off`, `on`, and `disabled` as the supported GPIO switch output states. Form selects and command buttons will use those values directly so payloads stay close to the API contract and avoid hidden conversions.

4. Expose runtime output through the adapter boundary.

   `output_state` is runtime state, not config. The backend should not place it under `config`, because `config.startup_state` and `config.safe_state` are persisted settings while the current output state changes at runtime. The REST adapter API should receive the runtime pointer alongside the persisted `DeviceRecord`, allowing `GpioSwitchDeviceApiAdapter` to serialize type-specific runtime data without making the controller know switch internals.

   Target JSON shape:

   ```json
   {
     "config": {
       "gpio_pin": 4,
       "startup_state": "off",
       "safe_state": "disabled",
       "restore_previous_state": true,
       "inverted": false
     },
     "output": {
       "state": "on"
     }
   }
   ```

   `output.physical_level` may be added later if needed for diagnostics, but the dashboard Power button should use logical `output.state` because inversion makes physical level unsuitable for user-facing state.

   Realtime updates should use the existing generic device snapshot flow. A `device.upsert` websocket message should carry the same device JSON shape as REST, including `output.state` when available. No separate switch-specific websocket topic is needed.

7. Use snapshot bootstrap, then websocket-backed storage updates for device state.

   The SPA should load the device registry and dashboard layout once on first open, then keep device state in Pinia stores as the primary UI source of truth. Realtime `device.upsert` and `device.remove` messages should merge into the device store without forcing a full `/api/devices` reload. A point `GET /api/devices/:id` remains available as an explicit refresh or fallback path on detail screens. Dashboard layout stays on the current GET/PUT contract for now and does not need websocket-driven sync.

5. Keep dashboard widgets compact while allowing compact type-specific actions.

   The base dashboard card renders the shared card shell and device name, and uses `effective_status` only for visual dimming. It must not render `Ready`, `!Ready`, or raw `effective_status` text. Type-specific widgets reuse the base card and may add compact controls through composition/slots. GPIO switch may add one compact Power button without adding metadata rows or increasing the card height.

   The dashboard Power button is intentionally a safe two-state shortcut. It switches `on` to `off` and `off` to `on` with explicit commands. If runtime `output.state` is `disabled` or unknown, the dashboard Power button is disabled; users must open the detail/Devices control surface to explicitly choose `on`, `off`, or `disabled`.

   Visual states:
   - `on`: active/success Power button.
   - `off`: inactive/muted Power button.
   - `disabled`: disabled/muted Power button.
   - `effective_status != ready`: dimmed card and disabled Power button.
   - dashboard edit mode: disabled controls; card interaction is reserved for drag/remove.

6. Mock API must mirror backend behavior closely enough for UI checks.

   Mock create/update must preserve GPIO switch config fields. Mock command handling must update the visible output state for `on`, `off`, and `disabled` and reject unsupported commands so UI validation can be tested in the browser.

## Risks / Trade-offs

- [Risk] Firmware may expose a field name different from the frontend expectation. → Mitigation: align frontend contracts with the implemented backend REST adapter before coding and verify against `http://192.168.1.240`.
- [Risk] Adding controls to compact dashboard cards would break layout requirements. → Mitigation: allow only one compact switch Power action and keep richer controls in details.
- [Risk] Mock behavior may diverge from firmware. → Mitigation: keep mock payloads using the same typed contracts and verify against both mock mode and ESP32 where practical.
