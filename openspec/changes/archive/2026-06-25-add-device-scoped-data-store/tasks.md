## 1. Storage Abstraction

- [x] 1.1 Define the device-scoped data store API and per-device namespace/key model.
- [x] 1.2 Implement typed load/save/remove helpers for `retained_state` and future data types.
- [x] 1.3 Add clear-by-device support so all scoped data for one `DeviceId` can be removed together.

## 2. Retained-State Migration

- [x] 2.1 Move retained-state persistence calls onto the new device-scoped store path.
- [x] 2.2 Add compatibility reads for the legacy retained-state store and rewrite successful loads into the new format.
- [x] 2.3 Remove legacy retained-state records only after the migrated payload has been written successfully.

## 3. Registry Lifecycle Integration

- [x] 3.1 Wire `DeviceRegistry::remove()` to clear all device-scoped data after a device is deleted.
- [x] 3.2 Update registry flush and load code so device-scoped data participates in the same lifecycle boundaries as other durable device state.
- [x] 3.3 Ensure retained-state dirty tracking still coalesces writes without rewriting unrelated device-scoped payloads.

## 4. Tests and Documentation

- [x] 4.1 Add unit tests for device-scoped isolation, deletion cleanup, and missing-data behavior.
- [x] 4.2 Add migration regression tests covering legacy retained-state upgrade paths and failure retention.
- [x] 4.3 Update the change documentation to describe the new storage model and its intended future `display_layout` use.
