## Context

Device registry records store type-specific configuration as a bounded binary `configPayload`; this is correct for runtime and NVS persistence. The problem is at the REST/UI boundary: OneWire bus edit currently makes the SPA construct the internal binary layout in `encodeOneWireBusConfigBlob()` and sends it as `payload` on `update_config`.

OneWire bus create already accepts a JSON `config` object, and the DS18B20 update adapter already follows the same external JSON pattern. The OneWire update path should use that adapter-owned conversion too.

## Goals / Non-Goals

**Goals:**

- Make OneWire bus `update_config` requests use a JSON `config` object containing `enabled`, `gpio_pin`, and `internal_pullup`.
- Keep binary `configPayload` encoding inside firmware adapters and registry persistence only.
- Remove the OneWire binary encoder from the SPA edit flow.
- Preserve the existing generic command endpoint and registry mutation behavior.

**Non-Goals:**

- Do not redesign the registry storage format, config structs, or NVS snapshot codec.
- Do not change OneWire scan commands or other runtime `payload` strings such as `custom: scan`.
- Do not change GPIO switch update format in this change unless needed by shared typings or tests.

## Decisions

1. OneWire update parsing lives in `OneWireBusDeviceApiAdapter::parseUpdateConfigRequest`.

   The adapter already owns OneWire create JSON parsing and binary encoding. Adding the update override keeps the controller and registry type-agnostic. The alternative was to let the SPA keep encoding bytes, but that leaks firmware layout into TypeScript and makes requests unreadable.

2. REST input uses `config`, internal mutation uses `configPayload`.

   The accepted request shape is:

   ```json
   {
     "command": "update_config",
     "config": {
       "enabled": true,
       "gpio_pin": 4,
       "internal_pullup": true
     }
   }
   ```

   The adapter parses and validates this JSON, then calls `encodeOneWireBusDeviceConfig()` before filling `DeviceConfigUpdateRequest`. The registry API remains unchanged.

3. Keep compatibility localized.

   If legacy binary `payload` support is retained, it stays behind the common fallback parser and is not used by the SPA. New tests should assert the JSON path, not require clients to understand the binary format.

4. SPA typings allow structured config on commands.

   `DeviceCommandRequest` already has optional `config`; the OneWire edit command should populate it and omit `payload`. Mock handlers should decode the same JSON shape as the firmware adapter.

## Risks / Trade-offs

- Existing external clients may still send binary `payload` for OneWire update → Keeping fallback behavior, if present, allows a transition while the SPA moves to JSON.
- Enabled state exists both as common device state and inside the OneWire config struct → The adapter should encode the intended JSON `enabled` value when supplied, matching the existing create/config JSON contract.
- Tests could accidentally keep passing through the old SPA encoder → Add focused assertions that OneWire edit commands contain `config` and do not contain `payload`.
