# Implementation Tasks: SPI Device Presence Check (Safe State-Preserving Probe)

## 1. Backend: GPIO-probe Interface with State Management

- [ ] 1.1 Create `src/devices/bus/spi/ISpiCsProbeDriver.h` with methods: `readCurrentState(pin, &mode, &value)`, `configureOutput(pin, level)`, `configureInputPullup(pin)`, `configureInputPulldown(pin)`, `readLevel(pin, &level)`, `restoreState(pin, mode, value)`, `release(pin)`
- [ ] 1.2 Create `src/devices/bus/spi/ArduinoSpiCsProbeDriver.cpp` with Arduino real impl + NullSpiCsProbeDriver fallback for native builds
- [ ] 1.3 Add singleton accessors `defaultArduinoSpiCsProbeDriver()` and `createArduinoSpiCsProbeDriver()`

## 2. Backend: SpiBusDevice Core Probe Logic

- [ ] 2.1 Add enums `SpiProbeOutcome` (Unknown/Detected/NotDetected/Inconclusive) and `SpiProbeMethod` (None/MisoActivity/CsPullHeuristic) to SpiBusDevice.h
- [ ] 2.2 Add struct `SpiProbeResult{csPin, outcome, method, checkedAtMs, ready}` to SpiBusDevice.h
- [ ] 2.3 Add members `csProbeDriver_` and `probe_` to SpiBusDevice class; add test constructor overload for injecting `ISpiCsProbeDriver&`
- [ ] 2.4 Implement `probeChipSelect(uint8_t csPin)` private method with collision check, state save/restore, and outcome determination
- [ ] 2.5 Implement collision detection: iterate dependents, call `spiChipSelectPin(csPin)` callback to detect pin-in-use
- [ ] 2.6 Implement `saveAndRestoreCsState()` helper: reads current pin mode (OUTPUT/INPUT) and level, returns struct
- [ ] 2.7 Implement `probeViaMisoActivity(uint8_t csPin)` private method: toggle CS within DependencyTransaction, observe MISO
- [ ] 2.8 Implement `probeViaCsPullHeuristic(uint8_t csPin)` private method: INPUT_PULLUP/PULLDOWN tests, compare readings
- [ ] 2.9 Extend `SpiBusDevice::handleCommand()` to dispatch `DeviceCommandType::CheckDevice` to `probeChipSelect()`
- [ ] 2.10 Extend `SpiBusDevice::writeDeviceJson()` to serialize `probe_` struct into runtime JSON

## 3. Backend: Command Type and Routing

- [ ] 3.1 Add `CheckDevice = 13` to `DeviceCommandType` enum in `src/devices/core/DeviceTypes.h`
- [ ] 3.2 Add `case DeviceCommandType::CheckDevice:` to `policyForCommand()` in `src/devices/registry/DeviceRegistry.cpp` (Delayed group)
- [ ] 3.3 Add `case DeviceCommandType::CheckDevice:` to exhaustive switch in `DeviceRegistry::command()` (freefall with SetStatus/Scan/SetOutput/ResetDiagnostics/Custom)

## 4. Backend: HTTP Controller Routing

- [ ] 4.1 Add `else if (std::strcmp(commandName, "checkDevice") == 0)` branch to `DeviceRegistryController::cmd()`
- [ ] 4.2 Parse `input["csPin"]` as integer, validate range [0..39], reject with HTTP 400 BAD_ARGS if invalid
- [ ] 4.3 Build `DeviceCommand{DeviceCommandType::CheckDevice, deviceId_, csPinText}` and call `registry_.command()`

## 5. Backend: REST Adapter Serialization

- [ ] 5.1 Extend `SpiBusDeviceApiAdapter::writeDeviceJson()` to serialize `runtime.probe` object with outcome, method, csPin, checkedAtMs, ready

## 6. Backend: Unit Tests

- [ ] 6.1 Add `FakeSpiCsProbeDriver` to `test/test_devices/test_spi_bus_device.cpp` with scriptable state and level readings
- [ ] 6.2 Test scenario 1 (free pin): probe with full GPIO control
- [ ] 6.3 Test scenario 2 (configured pin): probe with save/restore; verify device state unchanged post-probe
- [ ] 6.4 Test scenario 3 (collision): probe rejected if another device uses same pin
- [ ] 6.5 Test MISO-activity detects device when bytes differ
- [ ] 6.6 Test MISO-activity returns Inconclusive when bytes identical
- [ ] 6.7 Test CS pull-heuristic when misoPin < 0
- [ ] 6.8 Test probe rejected when bus not Ready or dependencyTransactionActive
- [ ] 6.9 Test probe result serializes to JSON
- [ ] 6.10 Run `scripts/test.sh`; verify all new tests pass

## 7. Frontend: Type Contracts

- [ ] 7.1 Add `SpiBusProbeSnapshot` interface to `portal-spa/src/api/contracts.ts`
- [ ] 7.2 Extend `SpiBusRuntimeSnapshot` with `probe?: SpiBusProbeSnapshot`
- [ ] 7.3 Extend `DeviceCommandRequest` with command `'checkDevice'` and optional `csPin?: number`

## 8. Frontend: Mock Handler Parity

- [ ] 8.1 Add `case 'checkDevice':` in `mockCommandDevice()` in `portal-spa/src/mock/handlers.ts`
- [ ] 8.2 Handler updates mock device `runtime.probe` with simulated outcome
- [ ] 8.3 Ensure WebSocket subscribers receive device-updated event

## 9. Frontend: Reusable Component

- [ ] 9.1 Create `portal-spa/src/components/devices/common/SpiChipSelectProbe.vue`
- [ ] 9.2 Props: `busDeviceId`, `csPin`, `disabled`; i18n label keys
- [ ] 9.3 Template: button with onClick handler, loading state, error alert, result chip
- [ ] 9.4 Script: manage `scanBusy` and `scanError` refs; subscribe to device registry store
- [ ] 9.5 Compute `selectedBusDevice` and read `runtime.probe` from store
- [ ] 9.6 Result chip: color by outcome (success=Detected, default=NotDetected, warning=Inconclusive); show method + timestamp
- [ ] 9.7 Tooltip/caption: "Heuristic only" warning for CS pull method; clear confidence indicators

## 10. Frontend: ST7735 Integration

- [ ] 10.1 Open `portal-spa/src/components/devices/display/st7735/St7735DeviceForm.vue`
- [ ] 10.2 Import `SpiChipSelectProbe` component
- [ ] 10.3 Insert `<SpiChipSelectProbe :bus-device-id="currentValue.spiBusDeviceId" :cs-pin="currentValue.chipSelectPin" :disabled="busy" />` next to CS pin field
- [ ] 10.4 Verify component receives draft (unsaved) csPin, not saved value

## 11. Frontend: i18n Strings

- [ ] 11.1 Locate locale files in `portal-spa/src/locales/`
- [ ] 11.2 Add keys: `device.fields.spiCheckDeviceButton`, `device.dialog.spiProbeDetected`, `device.dialog.spiProbeNotDetected`, `device.dialog.spiProbeInconclusive`, `device.dialog.spiProbeMisoActivity`, `device.dialog.spiProbeCsPullHeuristic`, `device.dialog.spiProbeHeuristicWarning`

## 12. Frontend: Smoke Test

- [ ] 12.1 Start dev: `cd portal-spa && pnpm dev`
- [ ] 12.2 Navigate to `http://127.0.0.1:5176/?mockMode=1&mockReset=1`
- [ ] 12.3 Open ST7735 device edit form; verify "Check Device" button visible
- [ ] 12.4 Enter unsaved CS pin; click button; verify mock response in result chip
- [ ] 12.5 Test three outcomes (detected/not_detected/inconclusive) via mock settings
- [ ] 12.6 Verify button disabled when no SPI bus selected

## 13. Integration & Full Build

- [ ] 13.1 Run `pio run -e esp32dev` (C++ compiles, no -Wswitch errors)
- [ ] 13.2 Run `cd portal-spa && pnpm build` (TypeScript check passes)
- [ ] 13.3 Run `scripts/test.sh` (all tests pass)
- [ ] 13.4 Manual hardware test (if available): flash, probe ST7735 with MISO configured; verify MISO-activity detection; disable MISO, verify fallback behavior
- [ ] 13.5 Verify WebSocket payload in browser DevTools: confirm `runtime.probe` in device-updated messages

## 14. Documentation & Code Comments

- [ ] 14.1 Add inline comment in SpiBusDevice.h: "probe_ is transient runtime state; never persisted"
- [ ] 14.2 Add comment in `probeChipSelect()`: "Saves CS state, probes with MISO-activity or CS pull, restores state; non-blocking; heuristic-only"
- [ ] 14.3 Comment in `ISpiCsProbeDriver`: "GPIO interface for safe CS-pin probing with state preservation"
- [ ] 14.4 SpiChipSelectProbe.vue doc: "Reusable probe button; works for any SPI consumer; result is heuristic-only"
