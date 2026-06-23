## 1. Firmware transfer bundle

- [x] 1.1 Define the versioned device setup bundle schema and staging validation model.
- [x] 1.2 Implement export serialization as JSON Lines with restore metadata and default secret redaction.
- [x] 1.3 Implement import parsing from the request file context, full-bundle validation, and atomic restore application.

## 2. Portal API endpoints

- [x] 2.1 Add dedicated REST endpoints for device setup export and import.
- [x] 2.2 Map validation, size-limit, and restore failures to the existing portal error envelope.
- [x] 2.3 Wire the transfer endpoints into the existing controller and registry persistence flow.
- [x] 2.4 Connect the import flow to the web-server request file context instead of buffered request bodies.

## 3. Devices page flow

- [x] 3.1 Add an export action on the Devices page that downloads the current device setup bundle.
- [x] 3.2 Add an import action with file selection and explicit restore confirmation.
- [x] 3.3 Handle import success and failure states in the Devices page UI.

## 4. Verification

- [x] 4.1 Add backend tests for bundle versioning, redaction, and import rejection cases.
- [x] 4.2 Add backend tests for successful restore and atomic failure behavior.
- [x] 4.3 Add portal or SPA coverage for the export/import UI actions and API contract.
