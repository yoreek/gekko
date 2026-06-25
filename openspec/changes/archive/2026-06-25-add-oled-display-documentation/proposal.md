## Why

OLED display support needs a documented contract before implementation so the firmware, registry, and UI all agree on how a display is modeled, how its layout is persisted, and how live sensor values are rendered. Without that contract, the UI and backend will drift into one-off behavior for each new display widget.

## What Changes

- Introduce an `oled-display` capability that describes how a display device is configured over I2C.
- Define how a display can own a persistent, versioned layout payload for pages, widgets, and text templates.
- Define how virtual display widgets can bind to existing devices such as temperature sensors, switches, and other data sources.
- Define the contract for value templating and refresh behavior so the UI can configure display content without hardcoded firmware rendering logic.
- Reserve `display_layout` as a device-scoped payload key for display-owned state, with JSON used only at the API boundary and binary persistence owned by the OLED runtime through the generic registry persisted-state lifecycle.

## Capabilities

### New Capabilities
- `oled-display`: Display device configuration, page/widget model, binding to data sources, and template-driven rendering contract.

## Impact

- Firmware device model for the new OLED display device.
- Device-scoped storage contract for `display_layout`.
- Portal UI forms and device detail surfaces for display configuration.
- Runtime data binding between displays and source devices such as sensors.
- Future Adafruit OLED library integration and I2C bus wiring.
