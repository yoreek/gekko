## Why

The firmware has one special-case retained-state store today, but new device-owned data needs the same lifecycle rules: load by device ID, save with bounded writes, and disappear when the owning device is deleted. A generic device-scoped store keeps that behavior in one place and gives OLED/display layouts a home without bloating the static device config blob.

## What Changes

- Add a device-scoped data store backed by NVS/Preferences and addressed by device ID plus data type.
- Migrate retained-state persistence from the current single-purpose store into the new device-scoped mechanism.
- Make device deletion clear all scoped data owned by that device so old retained or layout data does not accumulate as garbage.
- Reserve the mechanism for multiple typed payloads, starting with `retained_state` and later `display_layout`, without expanding the device config record.
- Keep controller-level config, dynamic device records, and device-scoped data as separate storage concerns.

## Capabilities

### New Capabilities
- `device-scoped-data-store`: A per-device storage mechanism for bounded typed payloads such as retained state and display layout, with migration and cleanup behavior.

### Modified Capabilities
- `device-registry`: Registry persistence and lifecycle behavior now include device-scoped data load/save, migration from the legacy retained-state store, and cleanup on device deletion.

## Impact

- `src/devices/registry/` storage and lifecycle code.
- `src/devices/core/` runtime hooks that save or load retained data.
- `src/platform/PreferencesConfigStorage.*` or related storage helpers if the new store needs a small typed wrapper.
- Unit tests for retained-state migration, per-device cleanup, and type isolation.
- Future OLED/display layout work can reuse the same device-scoped storage model instead of creating another special-case store.
