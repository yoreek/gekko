## Context

The current firmware already uses `Preferences`-backed NVS for controller config, device registry state, and retained runtime values. The retained-state path is still a single-purpose store, though, and it is not a good fit for the next wave of device-owned data such as display layouts. The project also cannot rely on key enumeration in `Preferences`, so cleanup needs to be designed around what NVS can do well: open a bounded namespace, write a few typed keys, and clear the namespace when the owning device is removed.

`DeviceRegistry` is already the central lifecycle owner for create, update, delete, flush, and runtime restore. That makes it the right orchestration point for device-scoped cleanup, while the storage helper itself should remain focused on typed data access and migration rules.

## Concrete Shape

- New storage helper: `DeviceScopedDataStore`
- Legacy source to migrate away from: `DeviceRetainedDataStore`
- Initial device-owned data types:
  - `retained_state`
  - `display_layout`
- Namespace naming:
  - `dev_<8hex>` where `<8hex>` is the lowercase zero-padded hex `DeviceId`
  - Example: `DeviceId 42` uses namespace `dev_0000002a`
- Payload keys inside a device namespace:
  - `retained_state`
  - `display_layout`

Each payload remains a bounded typed blob owned by its own codec. The store is responsible for selecting the namespace and key, not for interpreting widget or sensor content.

## Goals / Non-Goals

**Goals:**
- Replace the single-purpose retained-state store with a generic device-scoped data mechanism.
- Keep device-owned auxiliary data bounded, typed, and addressable by `DeviceId`.
- Migrate existing retained-state data into the new mechanism without breaking boot or requiring a full NVS erase.
- Ensure device deletion clears all data owned by the deleted device.
- Leave room for later device-owned payloads such as `display_layout`.

**Non-Goals:**
- Reworking the main device registry index/record storage format.
- Introducing a filesystem-backed data store for device-scoped payloads.
- Adding display rendering behavior or OLED device UI in this change.
- Turning NVS into a general key enumeration database; the design must work within `Preferences` limits.

## Decisions

- Use a per-device namespace with explicit typed keys.
  - Each device gets its own bounded namespace, and named data types such as `retained_state` and `display_layout` live as keys inside that namespace.
  - This makes cleanup a single namespace clear when a device is deleted.
  - Alternative: one global namespace with per-device key prefixes. Rejected because `Preferences` does not offer the kind of enumeration we need for safe garbage cleanup.
  - Alternative: one blob per device that packs every type together. Rejected because one data type would overwrite another and migrations would become unnecessarily broad.

- Make `DeviceRegistry` the lifecycle orchestrator, not the storage owner.
  - The registry should decide when scoped data must be loaded, flushed, migrated, or deleted.
  - The storage helper should know how to read/write/clear data for one device and type, but it should not know registry rules or device-specific retention policy.
  - Alternative: keep a single retained-state-only wrapper and add separate stores for new data types. Rejected because that spreads cleanup and migration logic across multiple one-off helpers.

- Migrate retained state lazily and compatibly.
  - On first access, the new store should read the legacy retained-state record, write it into the new device-scoped location, and then remove the legacy entry after the new write succeeds.
  - This keeps boot working even when only some devices touch retained state after an upgrade.
  - Alternative: one-time boot migration scan. Rejected because the project cannot rely on cheap namespace enumeration and because lazy migration is bounded to real devices.

- Clear scoped data only after the delete commit point is durable.
  - `DeviceRegistry` remains the lifecycle authority and should request scoped cleanup only after the device deletion is committed in the same way it commits other durable registry changes.
  - Immediate delete can clear the namespace before returning success.
  - Delayed delete should track a pending cleanup set and clear device scopes on the flush path after the delete record is durable.
  - Alternative: clear the namespace as soon as delete is requested. Rejected because a later persistence failure would leave the registry and scoped data out of sync.

- Keep data type names stable and future-facing.
  - `retained_state` is the first required type.
  - `display_layout` is the next obvious type and should fit without changing the storage model.
  - Alternative: hard-code retained state only and add display layouts later with a separate storage shape. Rejected because that recreates the current special-case sprawl.

## Risks / Trade-offs

- [Risk] Device-scoped namespaces can accumulate if deletion cleanup is missed.
  [Mitigation] Route cleanup through `DeviceRegistry::remove()` and cover delete-path tests that verify all scoped keys are cleared.

- [Risk] Migration code can temporarily duplicate retained data during an upgrade.
  [Mitigation] Only remove the legacy record after the new record has been written successfully.

- [Risk] Changing runtime load/save hooks can ripple across switch-like devices.
  [Mitigation] Keep the public runtime contract narrow and adapt the implementation behind a compatibility layer during migration.

- [Risk] A generic store may become too permissive and start absorbing unrelated controller config.
  [Mitigation] Keep the store device-scoped and refuse to mix controller-level config with device-owned payloads.

## Migration Plan

1. Introduce the new device-scoped data abstraction and keep it side-by-side with the legacy retained-state path.
2. Add retained-state read compatibility so existing devices can load legacy data and rewrite it into the new store.
3. Switch retained-state writes to the new store after migration succeeds.
4. Teach `DeviceRegistry` to clear all scoped data on device deletion.
5. Remove the old retained-state-only helper once all callers and tests use the new store.

## Open Questions

- Should `display_layout` be implemented in this change or only reserved as the first non-retained data type?
- Do we want a boot-time scavenger for legacy keys, or is lazy migration on access sufficient?
