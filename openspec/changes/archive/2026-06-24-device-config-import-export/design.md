## Context

The firmware already persists device and controller data internally, and parts of the codebase already use bounded JSON serialization for configuration. What is missing is a user-facing transfer flow that can export a complete device setup, move it between devices, and restore it safely without manual recreation.

This change touches firmware persistence, REST endpoints, and the portal SPA. The design must keep memory use bounded, avoid partial writes, and preserve the current registry and configuration validation rules.
The transfer bundle will be JSON Lines so export and import can work incrementally, and import will consume the file through the shared web-server request file context rather than a buffered request body.

## Goals / Non-Goals

**Goals:**
- Export a complete device setup as a versioned, bounded JSON Lines bundle.
- Import that bundle safely and atomically.
- Preserve stable device identity, dependency links, and supported config versions during restore.
- Redact secrets from normal export output.
- Expose the flow through portal API endpoints and the Devices page.
- Reuse the web-server upload context so imported bundles arrive as files in request context.

**Non-Goals:**
- Add arbitrary partial patch import for a subset of fields.
- Add a new external storage backend or archive format.
- Change the existing runtime control model outside of what restore needs.
- Implement a secure secret backup mode unless it is explicitly required later.

## Decisions

### Use a versioned JSON bundle
The transfer artifact will be a JSON Lines document with an explicit bundle version in a leading envelope record and metadata for restore.

Why:
- The firmware already has JSON serialization infrastructure.
- JSON is inspectable, debuggable, and easy to test.
- JSON Lines lets export and import process records incrementally while keeping the format inspectable and debuggable.
- A leading envelope record can carry controller settings, registry snapshots, and future metadata without a new file format.

Alternatives considered:
- Binary archive: rejected because it is harder to debug, inspect, and evolve.
- Multiple files: rejected because restore would become order-dependent and harder to validate atomically.
- Single monolithic JSON object: rejected because it pushes large document parsing back into RAM.

### Export a full snapshot, not a partial patch
Export will capture the full device setup needed for restore, including registry records and required configuration metadata. Import will treat the bundle as a restore snapshot rather than a generic incremental update.

Why:
- Full snapshots give deterministic restore behavior.
- They avoid conflict resolution between imported data and existing device state.
- They fit backup/restore expectations better than a partial patch API.

Alternatives considered:
- Merge-by-default import: rejected because it creates ambiguous ownership rules for names, IDs, and dependencies.
- Field-level patch import: rejected because it is too close to the live mutation APIs and does not satisfy backup/restore needs.

### Parse into staging state before applying
Import will deserialize and validate into staging structures first, then apply the bundle only after all checks pass.

Why:
- Prevents partial mutation if validation fails partway through.
- Keeps the live registry unchanged on error.
- Makes rollback behavior straightforward: reject the bundle before any state is committed.

Alternatives considered:
- Streaming mutation during parse: rejected because it risks partially applied imports.

### Import consumes the uploaded bundle file from request context
The transfer controller will read the uploaded bundle through the shared request file context rather than expecting the whole body in memory.

Why:
- Keeps transfer import aligned with the web-server upload infrastructure.
- Avoids body buffering and lets the web-server manage tmp lifecycle.
- Makes the transfer action compatible with the same file semantics as other controllers.

Alternatives considered:
- Direct body buffering: rejected because the bundle can be larger than request memory limits.
- Separate raw upload path: rejected because the controller layer already needs request-scoped files.

### Redact secrets from normal exports
Normal exports will exclude or redact secrets such as WiFi credentials unless a separate secure backup mode is explicitly designed later.

Why:
- Prevents accidental leakage when users share backups.
- Matches the existing security posture for exported configuration data.

Alternatives considered:
- Export secrets by default: rejected because it is unsafe for routine backups.

### Add dedicated transfer endpoints and Devices page actions
The portal API will expose explicit export and import endpoints for the bundle, and the Devices page will provide user actions to trigger them.

Why:
- Keeps the flow discoverable in the existing device-management area.
- Avoids overloading the existing device CRUD endpoints with backup semantics.

Alternatives considered:
- Separate settings page: rejected because the feature is tied to device management and registry state.

## Risks / Trade-offs

- [Large bundle memory pressure] -> Bound bundle size, process JSON Lines incrementally, and reject oversized imports before allocation grows beyond the configured limit.
- [Accidental overwrite of live registry] -> Require explicit confirmation in the UI and use replace-only restore semantics.
- [Version mismatch between bundle and firmware] -> Include bundle version and schema metadata, and reject unsupported versions with a clear error.
- [Secret leakage in backups] -> Redact secrets by default and document the export behavior clearly in the UI.
- [Partial persistence after import failure] -> Stage and validate the entire bundle before applying any mutation to the live registry or persisted storage.

## Migration Plan

1. Add bundle codec and staging model for export/import.
2. Wire the firmware transfer endpoints to the codec and request file context.
3. Add Devices page controls and a confirmation flow for import.
4. Add or update automated tests for export, import, validation, size limits, and secret redaction.
5. Roll out export first if needed, then enable import after validation and restore semantics are covered by tests.

## Open Questions

- Should the import flow support replace-only restore only, or also a merge mode later?
- Should the bundle include controller settings, device registry state, or both by default?
- Do we need a future secure backup mode that retains secrets for offline migration?
