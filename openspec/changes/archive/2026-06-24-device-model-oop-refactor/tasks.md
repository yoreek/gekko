## 1. Device model layer

- [x] 1.1 Make each concrete device model own its flat create draft, config normalization, config encoding, and payload building behavior.
- [x] 1.2 Add typed output snapshot ownership to the concrete runtime/model boundary so device-specific output shape is explicit instead of a generic mixed bag.

## 2. Frontend create flow

- [x] 2.1 Keep the device create dialog as orchestration only: shared draft, active type-specific form, and model-owned submit payload generation.
- [x] 2.2 Move type-specific create validation and default values into the per-device create form components and their base form model.

## 3. Runtime and UI consumers

- [x] 3.1 Update detail and widget consumers to read typed runtime output through the owning device model or typed accessors.
- [x] 3.2 Align device-specific locale keys and status labels under the matching device namespaces.

## 4. Verification

- [x] 4.1 Run targeted typecheck/tests for the portal SPA and fix regressions introduced by the refactor.
- [x] 4.2 Review the remaining device types and mark any follow-up work that still requires conversion to the new model ownership pattern.
