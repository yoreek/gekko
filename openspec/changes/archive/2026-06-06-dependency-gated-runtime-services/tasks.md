## 1. WiFi Dependency Readiness

- [x] 1.1 Add explicit WiFi/network readiness APIs or a narrow dependency interface for network stack readiness, station readiness, and setup AP readiness.
- [x] 1.2 Ensure readiness state is driven by `WifiManager`/driver lifecycle without requiring `App::begin()` to call service ticks.
- [x] 1.3 Add host tests covering initial not-ready state, network stack ready state, station ready state, and setup AP ready state.
- [x] 1.4 Fix `ArduinoWifiDriver::startSetupAp()` so `setupApActive_` is set only after `WiFi.softAP(...)` succeeds and is cleared on failed AP startup.
- [x] 1.5 Add host tests covering failed setup AP startup and invalid setup AP IP so `setupApReady` cannot become true incorrectly.

## 2. Portal Service Lifecycle

- [x] 2.1 Convert `PortalServer` startup into configuration plus tick-driven lifecycle states such as idle, waiting for dependency, starting, running, and faulted where practical.
- [x] 2.2 Ensure `PortalServer::begin()` no longer calls `AsyncWebServer::begin()` before WiFi/network stack readiness is true.
- [x] 2.3 Start the async HTTP backend from `PortalServer::tick(now)` only after dependency readiness is satisfied.
- [x] 2.4 Keep captive DNS separately gated by setup AP active state and valid setup AP IP.
- [x] 2.5 Add bounded diagnostics or test-visible state for portal dependency wait and startup/fault paths.
- [x] 2.6 Keep the HTTP server running across AP-to-STA and STA-to-AP transitions while `networkStackReady` remains true, and stop/restart it only when the WiFi/TCP-IP stack dependency is explicitly lost.

## 3. OTA Service Lifecycle

- [x] 3.1 Convert `ArduinoOtaService` to an explicit dependency-gated lifecycle with idle/wait/start/running behavior.
- [x] 3.2 Start `ArduinoOTA.begin()` only after the device has usable WiFi/IP readiness on station or setup AP.
- [x] 3.3 Stop the OTA backend with `ArduinoOTA.end()` when the active OTA WiFi/IP target is lost or changes, then restart it after readiness returns.
- [x] 3.4 Add host tests covering OTA wait-before-WiFi, start-after-WiFi, WiFi-loss stop, and restart-after-reconnect behavior.

## 4. App Runtime Cleanup

- [x] 4.1 Remove runtime service `tick(...)` calls from `App::begin()`.
- [x] 4.2 Keep service `begin(...)` calls in `App::begin()` limited to configuration and dependency wiring.
- [x] 4.3 Keep all runtime service transitions advanced by `App::tick()` from the cooperative Arduino `loop()` path.
- [x] 4.4 Verify `App` does not contain service-specific branches for portal, OTA, MQTT-style, or other WiFi-dependent service readiness.

## 5. Tests And Verification

- [x] 5.1 Add or update host tests for boot ordering so portal and OTA cannot start their network backends before dependencies are ready.
- [x] 5.2 Run `scripts/test.sh`.
- [x] 5.3 Run PlatformIO firmware build for `esp32dev`; do not treat `esp32dev_ota` as a separate compile verification target unless OTA upload settings changed or an actual OTA upload check is being performed.
- [x] 5.4 Manually verify on ESP32: first boot provisioning, portal availability, captive DNS, AP-to-STA and STA-to-AP transitions without unnecessary HTTP restart, WiFi stop/down recovery, station reconnect, development OTA, and no `Invalid mbox` crash at boot.
- [x] 5.5 Keep mobile provisioning BLE-only; do not use WiFiProv SoftAP transport because setup AP and HTTP portal are owned by `WifiManager`/`PortalServer`.
