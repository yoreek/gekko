## Context

The firmware already uses a numeric `DeviceTypeId`, and the only supported runtime type today is `DummyDevice` with `type_id = 1`. The frontend briefly drifted away from that model by introducing string-oriented type handling and by carrying a fallback device branch that no longer matched the supported catalog. This change aligns the portal UI, mock transport, and tests to the real device type model.

## Goals / Non-Goals

**Goals:**
- Keep the frontend device type contract numeric and aligned with firmware.
- Maintain a single canonical portal catalog entry for `DummyDevice`.
- Remove unsupported generic fallback rendering from the device detail modal.
- Avoid stale mock/localStorage data from the previous schema.

**Non-Goals:**
- Introducing any new firmware device types.
- Building a generic typed renderer framework for future device families.
- Changing firmware registry storage or binary record layout.

## Decisions

- Use numeric `type_id` as the canonical frontend device type identifier.
  - Rationale: the firmware already exposes numeric type identifiers, and keeping the same representation avoids a translation layer in the portal.
  - Alternative considered: string keys such as `dummy`.
  - Rejected because it would duplicate an already-established firmware identity and complicate the create/detail flow.

- Keep the frontend catalog explicit and minimal.
  - Rationale: the app currently supports one type only, so the portal should present one typed option instead of an open-ended selector.
  - Alternative considered: a free-form text input or a generic type dropdown.
  - Rejected because both imply unsupported device families and create avoidable validation paths.

- Remove generic fallback rendering from the device detail modal.
  - Rationale: the UI should render the actual supported `DummyDevice` panel rather than pretending to support an undefined generic device path.
  - Alternative considered: retain a generic branch for future device types.
  - Rejected because it adds dead UI surface and confuses the current contract.

- Reset mock persistence by bumping the mock storage key.
  - Rationale: stale stored data from the previous schema can produce invalid runtime state and misleading tests.
  - Alternative considered: perform an in-place migration of the old localStorage blob.
  - Rejected because the old data shape is not worth preserving and the new seed is deterministic.

## Risks / Trade-offs

- [Risk] Old mock/localStorage data disappears after the schema bump -> Mitigation: the seed data is deterministic, and developers can recreate state quickly.
- [Risk] Future device types will require a catalog update in both code and specs -> Mitigation: keep the catalog centralized and add new entries through the same path.
- [Risk] The UI may look narrower because it no longer offers a generic fallback -> Mitigation: this is intentional while only `DummyDevice` is supported.

## Migration Plan

1. Deploy the frontend changes with the numeric type catalog and updated mock storage key.
2. Let existing mock/localStorage state reset naturally through the new storage version.
3. Verify smoke coverage against the built preview.
4. If a rollback is needed, restore the prior frontend revision and storage key handling together so state shape and code match again.

## Open Questions

- When the next device type is added, should the catalog continue to live only in the frontend, or should it be sourced from the backend?
- Should the portal eventually expose a user-visible type selector only after more than one supported type exists?
