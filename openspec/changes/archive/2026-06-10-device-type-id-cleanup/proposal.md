## Why

The frontend currently mixed several device type conventions: it briefly used string type keys, carried a generic device branch that no longer matched the supported model, and exposed technical persistence hints in the UI. The project already has a single supported device type in firmware (`DummyDevice` with numeric `type_id = 1`), so the portal should align to that contract and stop inventing unsupported type paths.

## What Changes

- Standardize the frontend device type contract on numeric `type_id` values.
- Keep `DummyDevice` as the only supported frontend catalog entry for now.
- Remove the generic device fallback branch from the device detail UI.
- Keep the dashboard focused on supported device types and typed `DummyDevice` behavior.
- Reset mock persistence state through a new storage schema key so stale pre-change data does not leak into the updated model.

## Capabilities

### New Capabilities
- `device-type-catalog`: defines the portal's supported device type catalog, label mapping, and create-flow selection behavior.

### Modified Capabilities
- `device-dashboard-ui`: remove unsupported fallback rendering and align the dashboard detail modal with the single supported typed `DummyDevice` model.

## Impact

- Frontend API contracts and device model normalization.
- Dashboard create dialog, detail modal, and type label rendering.
- Mock database seed and storage schema.
- Unit and smoke tests that assert device type behavior.
