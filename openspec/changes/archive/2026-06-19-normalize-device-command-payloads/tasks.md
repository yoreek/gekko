## 1. Firmware Command Contract

- [x] 1.1 Extend `DeviceCommandType` and command parsing for named public commands such as `scan` and `set_output`.
- [x] 1.2 Change controller parsing so migrated commands read named fields (`name`, `status`, `state`, `config`, `has_parent`, `parent_device_id`) instead of public `payload`.
- [x] 1.3 Reject legacy payload-only request shapes for migrated commands before registry or runtime mutation.
- [x] 1.4 Keep any required conversion to internal `DeviceCommand.payload` inside firmware controller/runtime boundaries only.

## 2. Firmware Config Adapters

- [x] 2.1 Remove DS18B20 update fallback to the base binary payload parser and require JSON `config`.
- [x] 2.2 Add OneWire bus JSON `config` update parsing and internal config payload encoding.
- [x] 2.3 Add GPIO switch JSON `config` update parsing and internal config payload encoding.
- [x] 2.4 Change the base adapter update parser to reject unsupported typed config updates instead of reading binary `payload`.

## 3. SPA Command Requests

- [x] 3.1 Update `DeviceCommandRequest` typings to model structured command fields and remove generic `payload` from migrated command call sites.
- [x] 3.2 Change rename, set-status, OneWire scan, DS18B20 scan, switch output, and set-parent callers to send named fields.
- [x] 3.3 Change DS18B20, OneWire, and GPIO switch edit flows to send JSON `config` objects.
- [x] 3.4 Remove frontend binary config encoders for OneWire and GPIO switch once no callers remain.
- [x] 3.5 Update mock handlers to match the production structured command contract and reject migrated legacy payload-only shapes.

## 4. Tests And Verification

- [x] 4.1 Add firmware REST/controller tests for structured rename, set-status, scan, set-output, set-parent, and typed config updates.
- [x] 4.2 Add negative firmware tests proving migrated legacy `payload` request shapes are rejected.
- [x] 4.3 Add SPA tests proving command builders and controls send named fields and JSON config objects.
- [x] 4.4 Run `scripts/test.sh` and fix firmware/native regressions.
- [x] 4.5 Run the project SPA verification command for changed frontend files.
- [x] 4.6 Review that binary `payload` remains only in internal persistence, retained state, websocket envelopes, or explicitly non-command contexts.
