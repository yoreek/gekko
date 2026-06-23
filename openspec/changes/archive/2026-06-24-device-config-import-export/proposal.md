## Why

Users need a reliable way to back up a working device setup and restore it on the same device or another unit without recreating every device and setting by hand. Today the firmware already persists configuration and registry data internally, but it does not define a complete user-facing export/import workflow for transferring a device setup end to end.

## What Changes

- Add a device configuration transfer flow that lets users export and import a full device setup as a bounded JSON Lines bundle.
- Include device registry entries, per-device configuration, and required metadata in the export so restores can preserve stable device identity and supported version information.
- Validate imported data before applying it, including schema version, payload size, device count limits, and type compatibility.
- Reuse the web-server upload request context so imported bundles arrive as files instead of buffered request bodies.
- Preserve existing non-exported secrets by redacting or excluding them from normal exports unless a secure backup mode is explicitly defined.
- Add portal API endpoints for exporting and importing the configuration bundle.
- Add Devices page controls for initiating export and import from the UI.
- **BREAKING**: imported bundles may replace or merge the existing registry according to the restore rule defined by the new spec, rather than being treated as a partial ad hoc update.

## Capabilities

### New Capabilities
- `device-config-transfer`: full device setup export/import workflow, JSON Lines bundle format, validation rules, and restore behavior.

## Impact

- Firmware config serialization and persistence code in `src/config/` and registry storage paths.
- Portal REST API routing and request handling for export/import endpoints.
- Web-server request file context used by import handlers.
- Portal SPA Devices page, dialogs, and store actions for backup and restore flow.
