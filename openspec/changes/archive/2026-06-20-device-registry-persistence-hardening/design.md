## Context

The registry persists an index plus individual device records in the same NVS-backed namespace. The previous implementation wrote the index first, which was safe only when all subsequent record writes succeeded. If the process stopped between the index write and the record writes, the next boot could see an index entry that had no backing record. The index must also stay bounded and binary, not a heap string/vector assembled as an unbounded serialized value.

## Goals / Non-Goals

**Goals:**
- Make registry persistence resilient to partial failure during save.
- Keep registry index persistence memory bounded by the project device-count limit and avoid heap strings/vectors for the index path.
- Use the existing dirty-record model for selective record writes during delayed flush.
- Keep load behavior unchanged except for handling the safer on-disk shape produced by the new commit order.
- Remove the temporary copied device list from the `/api/devices` controller path.

**Non-Goals:**
- Reworking the registry into a fully transactional storage layer.
- Changing the scope of dirty tracking beyond the existing dirty index / dirty record model.
- Lifting storage, flash lifetime, or business-level device-count limits; the requirement is that RAM usage does not scale with index length.
- Optimizing unrelated response serialization or runtime copy paths outside the device registry list endpoint.

## Decisions

- Persist the registry index as a bounded binary record.
  - The index storage format keeps only fixed metadata plus a fixed entry array capped by `kMaxDynamicDevices` and `kMaxRegistryIndexBytes`.
  - Each index entry contains the device id and type id; the record key is derived from the device id, so no variable-length key string is stored in the index entry.
  - The implementation must not build the index as a heap `std::string`, heap `std::vector`, or unbounded serialized value before writing index data.
  - The current Preferences/NVS abstraction writes one binary value per key, so the chosen shape is a fixed-size trivially-copyable struct stored through `putBlob`.

- Write dirty device records before the index.
  - The index commit metadata is the last durable commit point.
  - Create writes the new record first, then writes staged index entries/pages and commits the index metadata; if the index metadata commit fails, an orphan record is acceptable because load uses only the committed index metadata.
  - Delete writes staged index entries/pages that exclude the removed device, commits the index metadata so the removed device is no longer loadable, and only then runs best-effort stale record cleanup.
  - Alternative: write the index first and repair on boot. Rejected because it preserves a recoverability gap and adds boot-time cleanup logic.

- Use dirty record IDs for selective config record persistence.
  - The current coordinator already identifies dirty index, dirty config, and dirty retained-state records.
  - Config-only flushes should write only dirty device records.
  - Rename is a record-only change because the display name is stored in the record, not the index.
  - Index writes should occur only when the coordinator reports a dirty index.
  - Alternative: keep `store.save(snapshot_)` for all config dirty work. Rejected because it ignores the dirty IDs and rewrites unrelated records.

- Keep dirty flag clearing coarse-grained after a flush attempt.
  - Dirty flags select the batch to write; they are not a recovery journal.
  - After attempting the selected batch, clear the corresponding dirty state as one unit and return any storage error to the caller.
  - Alternative: track per-record successful writes or keep failed dirty IDs pending inside one flush. Rejected because error handling is not meaningful enough to justify the extra state.

- Preserve existing load validation.
  - Existing load checks already reject missing records and corrupt snapshots.
  - Alternative: relax load behavior to tolerate missing records. Rejected because it hides persistence errors and can mask data loss.

- Version the stored registry format explicitly.
  - The registry namespace stores a firmware-owned format version separate from per-device config versions.
  - If the stored registry format is missing or unsupported, load clears the registry namespace and continues with an empty dynamic registry.
  - This is intentionally a compatibility reset, not a migration path, because old binary index/record layouts cannot be interpreted safely after format changes.

- Iterate device records directly when serializing the device list.
  - This removes the intermediate copied `std::vector<DeviceRecord>` from `DeviceRegistry::list()`.
  - Alternative: keep the copied list and optimize later. Rejected because the controller already only needs read-only iteration and per-record effective status.

## Risks / Trade-offs

- [Risk] A power loss during save can still leave a subset of records updated before the index commit.
  [Mitigation] The index commit metadata is written last, so the previous committed registry remains the visible recovery point until the new index generation is durable.

- [Risk] Writing records and staged index entries before the index metadata commit can briefly increase write amplification on failed saves.
  [Mitigation] The scope is bounded to the dirty records already selected by the coordinator.

- [Risk] Selective record saves can leave stale orphan records after deletion.
  [Mitigation] Treat the index as the authoritative record set on load and remove stale records only after the new index is successfully committed.

- [Risk] A create can leave an orphan record if the record write succeeds and index commit fails.
  [Mitigation] Treat orphan records as harmless because load only reads records referenced by the committed index.

- [Risk] A write failure can clear dirty state even though some writes did not persist.
  [Mitigation] Return the error, rely on committed index metadata for boot recovery, and avoid adding partial dirty-state recovery logic.

- [Risk] Existing tests may assume the previous write order.
  [Mitigation] Add regression tests for partial-save failure and successful recovery with the new commit order.

- [Risk] Changing the device list iteration API can ripple into the controller and tests.
  [Mitigation] Keep the new API minimal, read-only, and easy to adapt at the JSON serialization boundary.
