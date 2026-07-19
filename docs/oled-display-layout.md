# OLED Display Layout Persistence

This document fixes the current OLED layout contract in the firmware.

## Scope

- `layout` is part of the public OLED device API payload.
- JSON is used only at the REST boundary.
- The runtime keeps layout as `OledDisplayLayoutRecordV1`.
- Persistence uses the device-scoped `display_layout` binary payload.
- Text-rendering notes live in [oled-text-rendering-notes.md](./oled-text-rendering-notes.md).
- Placeholder behavior lives in [display-text-placeholders.md](./display-text-placeholders.md).
- Compiled placeholder state is transient runtime cache data and is not persisted in the layout payload.

## Actual Flow

### Create

1. The client sends `config.layout` as JSON in the OLED create payload.
2. `OledDisplayDeviceApiAdapter` parses the JSON layout and encodes it into a binary sidecar blob.
3. `DeviceRegistryController` creates the device through the normal device create flow.
4. After the device gets a real `deviceId`, the controller calls `DeviceRegistry::applyPersistedStateUpdate(...)`.
5. `OledDisplayDevice` decodes the binary blob into `layout_` and normalizes `layout.deviceId` to its own runtime `deviceId()`.
6. `DeviceRegistry::flushNow()` calls the generic persisted-state save hook and the runtime stores `layout_` under `display_layout`.

### Update

1. The client sends `config.layout` as JSON in the standard `updateConfig` command payload.
2. `OledDisplayDeviceApiAdapter` parses that JSON and encodes a binary sidecar blob into `DeviceConfigUpdateRequest`.
3. `DeviceRegistryController` runs the normal `updateConfigAndDeps(...)` path for the main device config blob.
4. After config update succeeds, the controller calls `DeviceRegistry::applyPersistedStateUpdate(...)`.
5. `OledDisplayDevice` decodes the binary blob into `layout_`.
6. `DeviceRegistry::flushNow()` persists the updated `layout_` as binary.

### Boot Reload

1. `DeviceRegistry::begin(...)` recreates runtimes from registry records.
2. If a runtime implements `IDevicePersistedState`, the registry calls `loadPersistedState(...)`.
3. `OledDisplayDevice` loads `display_layout` from `OledDisplayLayoutStore`.
4. The store returns binary data, the codec decodes it into `OledDisplayLayoutRecordV1`, and the runtime keeps that struct in `layout_`.

### Delete

1. `DeviceRegistry::remove(...)` calls the runtime's generic `clearPersistedState(...)`.
2. The device-scoped `display_layout` payload is cleared together with the device.

## Ownership

- `OledDisplayDeviceApiAdapter`
  - owns `JSON <-> binary sidecar` conversion
  - does not write to storage
  - does not own runtime state

- `DeviceRegistryController`
  - runs the standard create/update device command flow
  - applies opaque persisted-state sidecar through generic registry API
  - does not know OLED layout schema

- `DeviceRegistry`
  - owns generic persisted-state lifecycle
  - calls `loadPersistedState(...)`, `savePersistedState(...)`, and `clearPersistedState(...)`
  - does not know OLED-specific fields

- `OledDisplayDevice`
  - owns runtime `layout_` as `OledDisplayLayoutRecordV1`
  - decodes binary into struct
  - encodes struct to binary through `OledDisplayLayoutStore`
  - binds display placeholder AST state at runtime without persisting it

- `OledDisplayLayoutStore`
  - owns only typed binary persistence under `display_layout`
  - does not parse API JSON

## Placeholder Dependencies

Structured `dev.<deviceId>.<metricKey>` placeholders are content dependencies.

- Each referenced source device is stored as a `metric_source` dependency link on the display device.
- The runtime AST may cache direct runtime references for render speed, but those references are transient.
- The persisted dependency link is the durable source of truth for deletion protection and reload behavior.
- Layout text remains the source of truth for placeholder parsing and AST rebuild.

## Binary Model

The persisted payload is a compact binary blob with:

- `OledDisplayLayoutBinaryHeaderV1`
- one `OledDisplayLayoutBinaryPageHeaderV1` per page
- one `OledDisplayLayoutBinaryWidgetV1` per widget

The runtime struct is:

```cpp
struct OledDisplayLayoutRecordV1 {
    DeviceId deviceId{0};
    uint16_t recordVersion{1};
    uint8_t schemaVersion{kOledDisplayLayoutSchemaVersion};
    uint8_t activePageIndex{0};
    std::vector<OledDisplayLayoutPageV1> pages{};
};
```

`pages` and `widgets` are dynamic in RAM. The persisted representation is still binary and versioned.

## Rules

- No dedicated public `/layout` endpoint.
- No JSON persistence in NVS.
- No OLED-specific load/save loop in `App`.
- No direct storage writes from the OLED API adapter.
- Layout schema knowledge stays inside OLED codec/store/runtime code.
- Generic persisted-state lifecycle stays in `DeviceRegistry`.

## Current API Contract

The OLED API continues to expose layout under:

```json
{
  "typeName": "ssd1306",
  "config": {
    "name": "oled",
    "enabled": true,
    "deps": [
      {
        "role": "i2c_bus",
        "deviceId": 12
      }
    ],
    "i2cAddress": 60,
    "width": 128,
    "height": 64,
    "layout": {
      "schemaVersion": 1,
      "activePageIndex": 0,
      "pages": [
        {
          "id": "main",
          "widgets": [
            {
              "bindingKind": 3,
              "x": 0,
              "y": 0,
              "width": 64,
              "height": 16,
              "sourceDeviceId": 0,
              "metricId": 0,
              "text": "temp"
            }
          ]
        }
      ]
    }
  }
}
```

The same shape is returned back from the runtime, but the stored form is binary.
