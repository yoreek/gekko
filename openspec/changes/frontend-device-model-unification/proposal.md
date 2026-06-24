# Frontend Device Model Unification

## Summary

Unify the portal SPA device model around the canonical `record/config/runtime` structure documented in `docs/device-model-structures.md`.

The frontend currently uses a flattened legacy transport shape where identity, persisted config, runtime state, registry metadata, and websocket metadata are mixed in the same device object. This change defines the migration path to a single frontend model that mirrors the backend API model and websocket payload model.

## Goals

- Use one canonical `DeviceRecord<TConfig, TRuntime>` shape across API responses, websocket payloads, stores, mocks, and device-specific frontend models.
- Keep persisted fields in `config`, including `name`, `enabled`, and `deps`.
- Keep runtime fields in `runtime`.
- Keep identity and revisions in `record`.
- Use `typeName` in frontend/public contracts.
- Keep numeric `typeId` only as temporary compatibility or internal UI registry mapping while backend migration is incomplete.
- Remove `configVersion` from the frontend domain model.
- Do not expose `pendingPersistence` in frontend device records or UI state.
- Move device-specific frontend models to camelCase.

## Non-Goals

- Do not redesign the device UI.
- Do not change firmware persistence layout in this frontend-focused change.
- Do not introduce a second realtime device model.
- Do not keep parallel snake_case and camelCase frontend domain models.

## Canonical Shape

```ts
export interface DeviceRecordBase {
  id: number
  typeName: string
  configRevision: number
}

export interface DeviceRecord<TConfig, TRuntime> {
  record: DeviceRecordBase
  config: TConfig
  runtime: TRuntime
}
```

API and websocket device payloads must both normalize to this shape.
