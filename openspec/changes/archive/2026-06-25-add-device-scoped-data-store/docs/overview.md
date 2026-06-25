# Device-Scoped Data Store

The firmware needs a single storage mechanism for device-owned auxiliary data that does not belong in the main device config blob.

The technical contract is fixed in [technical.md](technical.md).

## What lives here

- `retained_state` for values that must survive reboot but are not configuration.
- `display_layout` for future OLED/layout data.
- Other small per-device payloads that should be deleted when the owning device is deleted.

## Why this exists

- The retained-state-only wrapper is too specialized and does not scale well to future device-owned data.
- `Preferences`/NVS is a good fit for bounded per-device payloads.
- `DeviceRegistry` already owns device lifecycle and is the right place to clear scoped data on delete.

## Lifecycle

1. Device data is loaded by `DeviceId` and type.
2. Writes are bounded and type-specific.
3. Deleting the device clears all scoped data.
4. Legacy retained-state entries are not supported in the final design.

## Fixed Names

- Store: `DeviceScopedDataStore`
- Legacy source: none
- Namespace format: `dev_<8hex>`
- Payload keys: `retained_state`, `display_layout`
