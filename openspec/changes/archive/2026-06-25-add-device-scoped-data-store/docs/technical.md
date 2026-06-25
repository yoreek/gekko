# Device-Scoped Data Store Technical Notes

This document fixes the technical vocabulary for the new device-owned storage mechanism.

## Class Names

- `DeviceScopedDataStore`
  - Owns device-scoped load/save/remove/clear behavior.
  - Chooses the NVS namespace from `DeviceId`.
  - Bridges typed payloads to `Preferences`/`IConfigStorage`.
- `DeviceRegistry`
  - Owns lifecycle orchestration.
  - Decides when device-scoped cleanup is committed.
- retained-state-only wrapper
  - Legacy helper being migrated away from.
  - Should not remain the public long-term owner of retained data.
- `DisplayLayoutStore`
  - Future device-scoped payload user for OLED layouts.

## Key Names

- Namespace format: `dev_<8hex>`
  - Example: `dev_0000002a`
- Payload keys inside that namespace:
  - `retained_state`
  - `display_layout`

## Data Ownership

- `retained_state` stores small restore values that are not part of the device config blob.
- `display_layout` stores the OLED layout payload for one device.
- The store does not interpret the payload content beyond versioned load/save and delete semantics.

## Example Flow

```text
switch output changes
  -> runtime marks retained state dirty
  -> DeviceRegistry schedules retained flush
  -> DeviceScopedDataStore writes:
       namespace = dev_0000002a
       key       = retained_state

device deleted
  -> DeviceRegistry commits delete
  -> DeviceScopedDataStore clears namespace dev_0000002a
  -> retained_state and display_layout are gone together
```

## Migration Rule

1. Try new `DeviceScopedDataStore` first.
2. If `retained_state` is missing, read the legacy retained-state record.
3. On successful read, write the new scoped payload.
4. Remove the legacy record only after the new write succeeds.

## Design Boundary

- The registry owns timing and lifecycle.
- The scoped store owns per-device persistence mechanics.
- Individual payload codecs own their binary schema.
- The store is not a generic registry, a filesystem, or a UI layout engine.
