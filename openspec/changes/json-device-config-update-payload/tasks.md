## 1. Firmware API

- [ ] 1.1 Add `OneWireBusDeviceApiAdapter::parseUpdateConfigRequest` declaration and implementation.
- [ ] 1.2 Parse `input["config"]` as OneWire JSON config, validate it with `parseOneWireBusDeviceConfigJson`, and encode it with `encodeOneWireBusDeviceConfig`.
- [ ] 1.3 Keep the registry mutation path unchanged by filling `DeviceConfigUpdateRequest.configPayload` and `configVersion` from the adapter.
- [ ] 1.4 Add REST adapter tests for successful OneWire JSON update, invalid JSON update rejection, and preservation of the internal binary config payload.

## 2. SPA Command Payload

- [ ] 2.1 Change OneWire edit command construction to send `config: { enabled, gpio_pin, internal_pullup }` instead of binary `payload`.
- [ ] 2.2 Remove the OneWire frontend binary blob encoder once no callers remain.
- [ ] 2.3 Update mock device command handling so OneWire `update_config` accepts the JSON config shape.
- [ ] 2.4 Add or update SPA tests proving OneWire edit sends `config` and omits binary `payload`.

## 3. Verification

- [ ] 3.1 Run `scripts/test.sh` and fix firmware/native regressions.
- [ ] 3.2 Run the project SPA verification command for changed frontend files.
- [ ] 3.3 Review that OneWire scan/custom commands still use their existing command-specific payloads and that registry/NVS binary storage remains internal only.
