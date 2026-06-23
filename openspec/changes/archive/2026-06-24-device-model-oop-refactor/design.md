## Context

The current device UI and model layer still leak type-specific logic across the create dialog, form helpers, and runtime snapshot handling. That forces the dialog to know too much about DS18B20, thermostat, and switch behavior, and it leaves runtime output as a broad mixed structure that is awkward to consume safely.

The codebase already has the right starting point for polymorphism: concrete device model classes, a model factory, and per-device create forms. This change makes that structure the source of truth instead of letting the dialog and helper modules reconstruct device behavior procedurally.

## Goals / Non-Goals

**Goals:**
- Make each concrete device model own its flat create draft, normalization, validation, and payload encoding.
- Keep shared registry fields on the same flat draft object as type-specific fields, with `deps` as the only relationship field.
- Remove type branching from the create dialog and move validation to the type-specific form/model layer.
- Treat runtime output as type-specific snapshot data owned by the concrete device runtime/model.
- Keep the migration incremental so existing device types can be converted without a full rewrite in one patch.

**Non-Goals:**
- Redesign the backend API surface unrelated to device model ownership.
- Add new runtime features or new device types.
- Change business rules for individual device types beyond relocating them into the owning model/form.

## Decisions

### 1. Use concrete device model classes as the ownership boundary
Each device type keeps its own `createDefaultCreateDraft`, config codec, validation, and payload encoder. The factory only resolves the correct instance. This matches the object model the code already uses and avoids a bigger switch-based dispatcher.

Alternatives considered:
- A shared procedural helper module. Rejected because it centralizes device knowledge again and recreates the same branching problem.
- A decorator-heavy registry. Rejected because the project already has a workable class/factory pattern and decorators would add indirection without removing domain complexity.

### 2. Keep the create draft flat
The create draft should expose common fields and type-specific fields on one object, rather than splitting into `common` and `config`. The dialog and forms can then share a single `v-model` object and the model can strip registry-only fields when building the payload.

Alternatives considered:
- Nested `common/config` state. Rejected because it duplicates shared fields and pushes the dialog toward merging structures manually.
- Separate payload objects per field group. Rejected because it makes form composition and type switching harder than a flat draft.

### 3. Let type-specific forms own validation
The create dialog should only compose the base form and the active type-specific form. Validation rules for dependencies and device-specific constraints belong in the active form component so the dialog stays generic.

Alternatives considered:
- Dialog-level validation with type checks. Rejected because it forces the dialog to understand every device type and makes changes fragile.
- A global form validator service. Rejected because the rules are already naturally attached to each device form and do not need a second abstraction.

### 4. Type-specific output snapshots should be owned by the concrete runtime
The shared runtime layer can keep the lifecycle and dependency wiring, but output extraction and serialization should be owned by the concrete runtime or device model. This keeps output shape aligned with the real device capability rather than a universal optional-field bag.

Alternatives considered:
- One universal output snapshot with many optional fields. Rejected because it loses type meaning and forces consumers to know device-specific semantics.
- Separate ad hoc output helpers in UI. Rejected because runtime serialization belongs closer to the device boundary.

## Risks / Trade-offs

- [Risk] Incremental conversion may leave temporary overlap between old and new helpers. → Mitigation: convert one device type at a time and keep the factory as the only resolution path.
- [Risk] The flat draft approach can look less structured at first glance. → Mitigation: keep ownership in the model class names and use small, typed interfaces for each device draft.
- [Risk] Runtime/output refactors may require matching frontend and backend changes. → Mitigation: keep snapshot serialization aligned with concrete device types and update the UI consumers together.
- [Risk] The create dialog can become too generic if forms are not strict enough. → Mitigation: require each type-specific form to expose its own rules and keep base/common validation in the shared base form.

## Migration Plan

1. Keep the concrete device model classes as the only place that knows how to build create defaults and payloads.
2. Convert the create dialog to a single flat draft and remove any remaining type-specific branching from submission.
3. Move validation rules into the shared base form and the type-specific forms.
4. Update runtime/output consumers to read the concrete snapshot shape for the active device type.
5. Convert the remaining device types to the same ownership pattern, one by one.

Rollback:
- Revert the new model/form wiring while keeping the existing factory and current API contracts intact.
- Because the change is incremental, individual device types can be rolled back independently if a specific form or codec regresses.

## Open Questions

- Should runtime output stay on a shared transport field with typed accessors per device, or should the API eventually split output snapshots into explicit per-device unions?
- Do we want the frontend factory to resolve only model instances, or also typed base-form metadata for each device type?
- Which device types should be converted first so the migration touches the smallest surface area while still proving the new ownership model?
