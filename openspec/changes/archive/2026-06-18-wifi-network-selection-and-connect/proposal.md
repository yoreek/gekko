## Why

The WiFi page can scan nearby networks, but the setup flow is still split across too many entry points: users need a clear way to pick or type an SSID, submit credentials, re-enter BLE config mode, and clear stored WiFi credentials when they want to fall back to AP mode. The backend already has most of the runtime pieces, but the portal needs to expose them as a single coherent setup surface.

## What Changes

- Turn the WiFi scan results into selectable network options on the portal page while still allowing manual SSID/password entry.
- Add connect, BLE config, and reset-credentials actions to the WiFi page so users can submit credentials, re-enter provisioning, or clear saved WiFi credentials from one place.
- Expose a WiFi credential-reset API path that clears stored credentials and lets the existing STA flow fall back to AP on the next cooperative tick.
- Show inline feedback for connection and reset actions so users can tell whether the requested WiFi action was accepted or rejected.
- Keep the scan flow, mock transport, and localization in sync with the new WiFi actions.

## Capabilities

### New Capabilities
- None.

### Modified Capabilities
- `portal-web-app`: extend the WiFi page so it supports manual SSID/password entry, scan-assisted selection, BLE config entry, and WiFi credential reset from the SPA.
- `portal-api-controllers`: expose WiFi credential reset alongside the existing scan, status, configure, and BLE config routes.
- `wifi-manager`: keep the runtime WiFi credential cache and STA fallback flow aligned with credential reset so AP recovery still happens through the cooperative state machine.

## Impact

- Affects the portal SPA WiFi page, frontend API usage, mock WiFi handlers, WiFi controller routes, and localized WiFi copy.
- Affects WiFi manager runtime credential handling so credential reset can fall back to AP through the existing cooperative flow.
- No storage format changes are required for this change.
