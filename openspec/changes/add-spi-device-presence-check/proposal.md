## Why

SPI bus lacks address enumeration (unlike I2C with address-scan). Users need a way to verify that an SPI device is actually present on a configured CS pin. Three realistic scenarios require safe handling:

1. **Device creation** (CS pin not yet in use) — full GPIO control available
2. **Device edit** (CS pin already configured) — must preserve and restore CS state
3. **Pin collision** (CS pin used by another device) — must detect and reject

Safe probe must temporarily toggle CS while preserving its original state, minimizing interference with active transfers.

## What Changes

- **Backend**: New `ISpiCsProbeDriver` GPIO interface for safe CS-pin control (read/write/configure with state preservation).
- **SpiBusDevice**: New `handleCommand(CheckDevice)` with:
  - CS-pin collision detection (reject if another device on same bus uses same pin)
  - Save current CS state → probe → restore CS state
  - MISO-activity test (primary) + CS pull-resistor heuristic (secondary if pin free or can be restored)
  - Return outcome + method used
- **REST layer**: New `DeviceCommandType::CheckDevice`; controller routing.
- **Runtime JSON**: Extend `SpiBusRuntimeSnapshot.probe` with result snapshot.
- **Frontend**: Reusable `SpiChipSelectProbe.vue` component; integrate into `St7735DeviceForm.vue`.
- **Tests**: Unit tests covering all three scenarios.

## Capabilities

### New Capabilities
- `spi-device-presence-check`: Safe, transient diagnostic probe for SPI devices on a specific CS pin. Handles three scenarios: free pin (full control), configured pin (state preservation), and pin collision (rejection). Uses MISO-activity (primary) and CS pull-resistor heuristic (secondary). Returns tri-state outcome (detected/not-detected/inconclusive) plus method.

### Modified Capabilities
<!-- None -->

## Impact

**Backend** (C++ / PlatformIO):
- `src/devices/bus/spi/` — new GPIO-probe interface + driver with state preservation
- `src/devices/bus/spi/SpiBusDevice.{h,cpp}` — collision detection + probe with save/restore
- `src/devices/core/DeviceTypes.h` — `DeviceCommandType::CheckDevice`
- `src/devices/registry/DeviceRegistry.cpp` — exhaustive switch updates (2 locations)
- `src/portal/controllers/DeviceRegistryController.cpp` — command routing with pin range validation
- `src/integrations/rest/spi_bus/SpiBusDeviceApiAdapter.cpp` — runtime JSON serialization
- `test/test_devices/test_spi_bus_device.cpp` — unit tests for all scenarios

**Frontend** (Vue / TypeScript):
- `portal-spa/src/api/contracts.ts` — `SpiBusProbeSnapshot` type, extend `SpiBusRuntimeSnapshot`
- `portal-spa/src/components/devices/common/SpiChipSelectProbe.vue` (new reusable component)
- `portal-spa/src/components/devices/display/st7735/St7735DeviceForm.vue` — integrate probe component
- `portal-spa/src/mock/handlers.ts` — mock `checkDevice` handler
- i18n keys for button/status labels

**No config version bumps required** — probe is pure transient runtime state.
