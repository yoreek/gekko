## Context

The firmware already supports both provisioning transports: BLE for mobile onboarding and SoftAP for HTTP portal onboarding. In practice, provisioning state is currently hard to observe from the logs, and there is no explicit admin path to re-enter BLE provisioning without clearing NVS or rebooting into a fresh state.

The portal is intended to remain available as a permanent admin interface, so provisioning re-entry must not be implemented by tearing down the web server or by coupling it to the WiFi connection lifecycle.

## Goals / Non-Goals

**Goals:**

- Emit explicit lifecycle logs for provisioning start and end.
- Provide an admin control path to re-enter BLE provisioning on demand.
- Preserve stored WiFi/NVS configuration when re-entering provisioning.
- Keep the portal always-on and avoid making provisioning re-entry depend on a full device reset.

**Non-Goals:**

- Redesign the full provisioning protocol or replace Espressif provisioning APIs.
- Remove SoftAP provisioning support.
- Change the WiFi connection model or NVS schema.
- Add authentication or role-based access control to the admin portal in this change.

## Decisions

1. Use the existing provisioning state machine and add explicit lifecycle hooks instead of introducing a second provisioning subsystem.

   This keeps the re-entry path aligned with the current cooperative loop and avoids duplicating transport/session management. The alternative would be a separate provisioning manager with its own lifecycle, but that would add unnecessary complexity and make state transitions harder to reason about.

2. Keep BLE and SoftAP provisioning as separate transports, but make BLE the explicit re-entry target for the new admin action.

   BLE is the mobile-first path that needs better observability and a clean manual restart. SoftAP remains available for HTTP-based setup, but the manual re-entry action should target BLE because that is the path that currently needs the most operational control. The alternative of forcing re-entry through SoftAP would not solve the mobile provisioning discovery issue.

3. Add explicit `PROV_START` and `PROV_END` logs at the provisioning boundary.

   These logs are the lowest-cost way to confirm that the provisioning service actually started and stopped. Relying on downstream WiFi events alone is ambiguous because WiFi connection success can happen after provisioning is already ending.

4. Expose provisioning re-entry through the admin portal as an explicit control action.

   This fits the existing always-on admin interface and makes it possible to recover from provisioning discovery issues without erasing NVS. The alternative of requiring NVS erase or a special boot mode is too coarse for field use.

5. Register `WiFi.onEvent()` once per provisioning session and make callbacks no-ops when `MobileProvisioning` is not active.

   This keeps event hookup stable and avoids repeated registration or teardown churn. The callback layer should only forward events when the provisioning session is live; inactive sessions must ignore them. The same pattern applies to `WifiManager` callbacks: register once, then guard on the current active state before mutating runtime flow. Provisioning start should be driven by the absence of stored credentials, with SoftAP brought up first only when the provisioning path requires it. For manual BLE re-entry, the device should first transition the network layer into provisioning fallback AP mode so the admin portal remains reachable, then restart the BLE provisioning session. Provisioning stop should be driven by successful credential acceptance, not by `WiFi connected`, so the provisioning session ends when it has done its job.

6. Keep the ESP32 Arduino Bluetooth controller reserved for provisioning by overriding the framework's weak `btInUse()` default in the firmware.

   On this platform, `initArduino()` can release BTDM memory early if Bluetooth is considered unused, which prevents BLE provisioning from starting and surfaces as `simple_ble_start enable controller failed 259`. The firmware fixes that by providing a strong `btInUse()` implementation in `src/platform/ArduinoBluetooth.cpp`, ensuring the framework does not free Bluetooth memory before `WiFiProv.beginProvision()` runs. This is a platform adapter concern, not a provisioning-state concern, so it stays at the Arduino boundary rather than inside the provisioning state machine.

## Risks / Trade-offs

- [Risk] Re-entering provisioning while WiFi is connected could create lifecycle races. → Mitigation: gate re-entry through the existing state machine and stop only the provisioning session, not the portal or NVS.
- [Risk] BLE provisioning may still be sensitive to platform-specific Bluetooth stack behavior. → Mitigation: keep the re-entry path isolated and observable with explicit logs so failures are easier to diagnose.
- [Risk] Admin re-entry without authentication can be abused on an open network. → Mitigation: defer access control to a later portal security change, but keep the control surface small and explicit for now.
- [Risk] Adding another portal action can increase UI complexity. → Mitigation: keep the control as a single action/button and avoid expanding the UI surface beyond the minimum needed for recovery.

## Migration Plan

1. Add provisioning lifecycle logs and the admin re-entry action behind the current portal.
2. Verify that BLE provisioning can be re-entered from the portal without clearing NVS.
3. Verify that existing stored WiFi credentials remain intact after re-entry.
4. If needed, add a rollback path by leaving the existing start/stop provisioning behavior in place and disabling only the new re-entry action.

## Open Questions

- Should the re-entry action be exposed as a dedicated API endpoint, a portal button, or both?
- Should the UI show the current provisioning transport and state explicitly?
- Should BLE provisioning re-entry be rate-limited to avoid repeated restarts?
