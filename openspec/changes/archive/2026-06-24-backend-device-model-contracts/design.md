## Context

The backend currently mixes several device representations: persisted registry records, REST snapshots, realtime payloads, and setup bundle exports. Some of those shapes are nested, some are flat, and runtime state is sometimes interleaved with persisted config. That makes it difficult to keep device serialization, mutation handling, and persistence aligned across the registry, API controllers, and realtime bridge.

The frontend has already converged on canonical nested contracts, so the backend must now align its own internal and external device models to the same record/config/runtime split.

## Goals / Non-Goals

**Goals:**
- Make backend device handling consistently use `record`, `config`, and `runtime` boundaries.
- Align API, realtime, registry, and setup bundle payloads with the canonical device model contracts.
- Keep persisted config separate from runtime output and status.
- Preserve device-type ownership of config encoding/decoding and validation.
- Remove flat export/import payloads that duplicate base fields outside the canonical record/config split.

**Non-Goals:**
- Rewriting device runtime logic or hardware behavior.
- Changing user-visible device functionality beyond payload/model shape alignment.
- Introducing a new transport or persistence backend.
- Renaming device families or changing their type ids.

## Decisions

### 1. Use a single canonical model family across backend surfaces
The backend should treat `DeviceApiRecord<TConfig, TRuntime>` as the REST-facing record, `DeviceRecord<TConfig, TRuntime>` as the shared realtime/frontend shape, and `DeviceSetupRecord<TConfig>` as the bundle transfer shape.

Rationale:
- This keeps the same identity/config split across all layers.
- It avoids repeated conversion between unrelated record forms.
- It makes backend and frontend payloads easier to reason about and test.

Alternative considered:
- Keep flat export/import records and continue mapping them into nested API/runtime objects. Rejected because it preserves duplicate representations and conversion churn.

### 2. Keep runtime state out of persisted setup bundles
Export/import bundles should carry only `record` and `config`. Runtime fields are reconstructed after import from registry state and device runtime initialization.

Rationale:
- Runtime output is transient and should not be transferred as persisted device data.
- A bundle containing only persisted identity/config is easier to validate and migrate.

Alternative considered:
- Serialize runtime alongside config for a “full snapshot” export. Rejected because it mixes volatile runtime with persisted state and complicates import semantics.

### 3. Delegate serialization ownership to device-type models
Concrete device model classes should own their config encode/decode, validation, and API serialization behavior. Shared base layers should only cover common fields and common runtime identity.

Rationale:
- This matches the object-oriented model already used by device families.
- It keeps type-specific config evolution localized.
- It reduces controller and registry branching.

Alternative considered:
- Centralize all serialization in the registry/controller layers. Rejected because it spreads device-specific knowledge into generic code and increases coupling.

### 4. Keep registry persistence contract separate from transport contracts
The registry should continue to persist device state in its own storage format, but its load/save adapters must map that state to the canonical nested record shape used by API and realtime contracts.

Rationale:
- Persistence can remain optimized for NVS/binary storage.
- Transport and storage concerns stay separated while still sharing the same field ownership rules.

Alternative considered:
- Make storage format identical to API payload format. Rejected because registry storage has different constraints, especially around binary payloads and revision handling.

## Risks / Trade-offs

- [Risk] Multiple specs and code paths must be updated together to avoid temporary contract drift. → Mitigation: implement the canonical record/config shape first, then update API, realtime, bundle, and tests in one pass.
- [Risk] Some legacy bundle or registry data may still be in the older flat shape. → Mitigation: support a migration path only where needed, then remove the legacy shape after the transition is complete.
- [Risk] More explicit contracts can expose missing runtime/config ownership in device models. → Mitigation: add focused tests per device family and keep type adapters small and typed.
- [Risk] The backend may need wider refactors than a single change can safely absorb. → Mitigation: keep the change scoped to model contracts, serialization boundaries, and persistence adapters; defer runtime behavior changes.

## Migration Plan

1. Introduce the canonical device model types and bundle record type in backend code.
2. Update REST controllers to emit and accept canonical nested device records.
3. Update realtime publishers and consumers to use the same nested record shape.
4. Update setup export/import to use `DeviceSetupRecord<TConfig>` and remove flat device bundle output.
5. Update registry adapters and type-specific model classes to own the encode/decode and persistence mapping.
6. Run backend and frontend contract tests together to verify that payload shape changes stay aligned.
7. If a rollback is needed, restore the previous adapter layer while keeping the underlying registry data unchanged.

## Open Questions

- Should the backend keep any temporary compatibility fallback for legacy flat setup bundles, or should the import change be strict once the backend rollout starts?
- Do we want a dedicated backend helper type for `DeviceSetupRecord<TConfig>` in firmware code, or should the bundle serializer use the same API record base types with a narrower export interface?
- Which device families need explicit migration tests first: dummy, GPIO switch, OneWire bus, DS18B20, or thermostat?
