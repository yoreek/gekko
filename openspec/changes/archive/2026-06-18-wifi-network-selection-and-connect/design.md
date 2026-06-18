## Context

The WiFi page already performs network scans and shows WiFi status, but it stops short of completing the setup workflow. Users can see nearby access points, yet the page does not fully cover the common setup paths: selecting a scanned SSID, entering credentials manually, re-entering BLE config mode, or clearing saved WiFi credentials so the device returns to AP mode on the normal cooperative flow.

The existing portal SPA already has WiFi scan/configure APIs, a BLE config API, mock transport, and localized strings. The change is therefore mostly a portal interaction update plus a small firmware-side credential-reset path, not a protocol redesign.

## Goals / Non-Goals

**Goals:**
- Let users pick a scanned WiFi network directly from the WiFi page or enter credentials manually.
- Submit WiFi credentials from the same page using the existing configure endpoint.
- Expose a BLE config action and a WiFi credential reset action from the same page.
- Keep scan, select, connect, BLE, and reset actions understandable on desktop and mobile.
- Preserve mock-mode and localization parity.

**Non-Goals:**
- No provisioning or captive-portal redesign.
- No new third-party dependencies for selection widgets or notifications.
- No unrelated WiFi policy changes such as retry timing, scan strategy, or credential validation rules.

## Decisions

1. Reuse the existing configure endpoint instead of adding a new WiFi submission flow.
   - Rationale: the backend already has the endpoint and request shape needed for credential submission, so the frontend should consume that directly.
   - Alternatives considered:
     - Introducing a new connect endpoint. Rejected because it duplicates the current contract.
     - Splitting credentials into a multi-step wizard. Rejected because the task is simple enough for one form.

2. Present scan results as selectable items in the WiFi page itself.
   - Rationale: the user is already on the setup page and should not need to jump to another view to choose a network.
   - Alternatives considered:
     - Keep scan results read-only and add a separate selection modal. Rejected because it adds friction.
     - Auto-submit the strongest SSID. Rejected because it makes the connection opaque and error-prone.

3. Keep the credential form editable even after selecting a scanned network.
   - Rationale: scan results can be stale or the user may need to connect to an SSID that did not appear in the current scan.
   - Alternatives considered:
     - Lock the SSID field after selection. Rejected because it removes flexibility and hurts troubleshooting.

4. Show inline connection feedback in the WiFi page instead of a separate toast system.
   - Rationale: the page already uses card-based sections, and inline feedback keeps the setup outcome close to the form that triggered it.
   - Alternatives considered:
     - Global snackbar notifications. Rejected because they are easier to miss on a setup page.

5. Implement WiFi credential reset as a clear runtime action instead of a manual state override.
   - Rationale: clearing credentials should be observable through the existing state machine flow rather than forcing a hidden one-off transition. The runtime credential cache is what the STA flow reads, so the reset path should clear that cache and let the station guards return to `Idle` and then `SetupAp`.
   - Alternatives considered:
     - Jumping directly to `SetupAp` with a manual state override. Rejected because it bypasses the normal cooperative flow and couples the reset action to one specific state.
     - Only clearing persistent storage. Rejected because the active runtime credential cache would still keep the STA branch alive until the next reload.

## Risks / Trade-offs

- [Risk] The WiFi page can become visually dense once scan results and connect form live together. -> Mitigation: keep the scan list and connect form as separate blocks with clear section spacing.
- [Risk] The selectable scan rows could confuse users if they look like buttons but do not clearly show selection. -> Mitigation: use an explicit selected state and a conventional form control for the SSID field.
- [Risk] Mock and real transport behavior could diverge on error handling. -> Mitigation: keep both paths aligned on the same request payload and response shape.
- [Risk] Inline feedback may get lost if the user scrolls. -> Mitigation: keep the feedback near the connect control and preserve the scan form order.
- [Risk] Credential reset could appear to do nothing if only persistent storage changes. -> Mitigation: clear the runtime WiFi credential cache as part of the reset path and let the station guards return the manager to `Idle` before AP fallback.
- [Risk] BLE config and credential reset are distinct intents but live on the same page. -> Mitigation: keep them as separate buttons with explicit labels and separate success/error feedback.

## Migration Plan

1. Update the WiFi page to treat scan results as selectable network options.
2. Add SSID/password form controls and wire the connect action to the existing WiFi configure API.
3. Add BLE config and credential reset buttons to the WiFi page and wire them to the existing/new WiFi API routes.
4. Keep mock handlers and localization aligned with the new WiFi actions.
5. Add a firmware-side credential reset path that clears the runtime credential cache and lets `CheckConnection` and `Connected` return through `Idle` to `SetupAp`.
6. Verify the page still supports scan refresh, manual SSID entry, BLE config, credential reset, and inline feedback.
7. Roll back by removing the extra actions and returning the WiFi page to scan/connect-only behavior.
