## Why

The current device layer mixes registry records, runtime output, and create/edit form state into one generic shape. That makes device-specific validation, payload generation, and snapshot rendering brittle and forces the UI to carry type checks that belong in the device models.

This change moves the codebase toward a per-device object model so each device type owns its own create draft, config codec, output snapshot, and validation rules. The goal is to make device behavior explicit, remove shared "bag of fields" logic, and make the UI delegate to the model classes instead of branching on type in the dialog layer.

## What Changes

- Introduce a typed, per-device model layer for create drafts, config encoding/decoding, output snapshots, and validation.
- Keep common registry fields separate from type-specific config fields and remove shared create-form logic that merges unrelated concerns.
- Make create forms inherit from a base form model and let each device form validate its own fields instead of centralizing type-specific checks in the create dialog.
- Keep runtime/output snapshots typed per device capability instead of treating all output fields as one universal mixed structure.
- Preserve registry and runtime contracts, but route normalization and payload building through the owning device model classes.
- **BREAKING**: device create/edit payload construction and output snapshot typing will change at the UI/model boundary, replacing generic mixed-field handling with per-device model methods.

## Capabilities

### New Capabilities
- `device-model-object-layer`: typed per-device create drafts, config codecs, output snapshots, and validation ownership for device models.

### Modified Capabilities
- `device-runtime-hierarchy`: runtime output snapshots must be exposed through typed per-device output models instead of one universal bag of optional fields.
- `portal-web-app`: the SPA device create/edit flows must delegate validation and payload creation to device-specific form/model classes.

## Impact

- Portal SPA device forms, create dialog, detail views, and model helpers.
- Frontend device model classes and registry resolution logic.
- API contracts for device output snapshots and create payload normalization.
- Firmware-side device serialization and runtime snapshot adapters where output shape needs to remain aligned with the new typed model layer.
