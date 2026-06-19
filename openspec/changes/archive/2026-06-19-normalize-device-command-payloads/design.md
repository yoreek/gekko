## Context

There are two different concepts currently named `payload`.

Internal firmware payloads are valid implementation details:

- `DeviceRecord.configPayload` is the bounded binary config snapshot stored by the registry and NVS.
- retained-state records use compact binary payloads.
- websocket envelopes use `payload` as a message body field.

The problem is the public device command request body for `/api/devices/:id/command`. It currently uses one generic string field for several unrelated meanings:

```json
{ "command": "rename", "payload": "Kitchen" }
{ "command": "update_config", "payload": "<binary bytes>" }
{ "command": "set_status", "payload": "fault" }
{ "command": "custom", "payload": "scan" }
{ "command": "custom", "payload": "state=on" }
```

That public API does not scale. Each command should expose its real fields directly.

## Goals / Non-Goals

**Goals:**

- Normalize the public device command API so each command has named JSON fields with one clear value per field.
- Remove frontend and external-client responsibility for binary config encoding.
- Reject legacy binary config payloads for typed `update_config`.
- Replace known public `custom` command strings with named commands.
- Keep existing internal registry/runtime code paths where they remain useful, with conversion hidden in firmware controller/adapters.

**Non-Goals:**

- Do not redesign registry/NVS binary storage.
- Do not remove `payload` from websocket envelopes, retained-state storage, or other internal/non-command contexts.
- Do not introduce blocking runtime flows or broad runtime refactors.
- Do not require a new endpoint if the existing `/api/devices/:id/command` route can carry the structured commands cleanly.

## Decisions

1. Public command requests are structured JSON.

   The public API should move from generic payload strings to named fields:

   ```json
   { "command": "rename", "name": "Kitchen" }
   { "command": "set_status", "status": "fault" }
   { "command": "scan" }
   { "command": "set_output", "state": "on" }
   { "command": "update_config", "config": { "gpio_pin": 4 } }
   ```

   The rule is simple: if a field has meaning, it gets its own JSON name. Avoid packed strings like `state=on`, `parent=123`, or binary data in public request bodies.

2. Binary config encoding is adapter-owned.

   For typed configs:

   ```text
   REST JSON config
        │
        ▼
   Type API adapter parser
        │
        ▼
   C++ config struct
        │
        ▼
   encode<Type>Config()
        │
        ▼
   DeviceConfigUpdateRequest.configPayload
        │
        ▼
   Registry / NVS binary storage
   ```

   The SPA and external clients never construct `configPayload`.

3. Legacy config payload fallback is removed.

   DS18B20 currently accepts JSON `config` but falls back to the base binary parser when `config` is missing. That fallback should be removed. OneWire and GPIO switch should receive explicit JSON update parsers. The base `IDeviceApiAdapter::parseUpdateConfigRequest` should reject config updates by default instead of accepting `payload`.

4. Known custom commands become named public commands.

   Existing runtimes can keep receiving internal `DeviceCommandType::Custom` if that avoids a runtime refactor, but the public controller should translate structured commands:

   ```text
   { command: "scan" }              -> internal Custom "scan" for OneWire bus
   { command: "set_output", state } -> internal Custom "state=<state>" for switch-like runtimes
   ```

   This keeps the wire contract clean without forcing every runtime to change at once.

5. Common commands get named fields.

   `rename` should parse `name`. `set_status` should parse `status`. `set_parent` already uses `has_parent` and `parent_device_id`; if internal code still needs `parent=<id>`, the controller can keep that conversion private.

## Risks / Trade-offs

- Existing clients using string `payload` will break -> This is acceptable for the portal API cleanup; tests should assert legacy shapes are rejected for migrated commands.
- Command parsing becomes more explicit in the controller -> The clarity is worth it because validation can now be per-command and field-specific.
- Internal runtime commands may still use compact strings for a while -> That is acceptable only as an internal bridge; it must not leak back to REST/SPA contracts.
- The current change grows beyond config update -> This is intentional. The new requirement is to plan the API shape for future device commands instead of repeating local one-off fixes.
