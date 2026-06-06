## Context

The previous lifecycle cleanup moved service-owned policy out of `App`, but `App::begin()` still performs an initial service tick sequence before the Arduino `loop()` starts. That sequence exists because `PortalServer::begin()` can crash when `AsyncWebServer::begin()` reaches AsyncTCP/LwIP before the WiFi/network stack is initialized.

This is a dependency-ordering problem, not an application orchestration problem. WiFi-dependent services need their own lifecycle gates so they start their network backends only after the required WiFi state is available. Future services such as MQTT would have the same class of failure if they create sockets or start TCP clients before WiFi readiness is true.

## Goals / Non-Goals

**Goals:**

- Remove duplicate service ticking from `App::begin()`.
- Keep all runtime service ticks in the cooperative `loop()` path through `App::tick()`.
- Make WiFi/network dependency readiness explicit and testable.
- Convert WiFi-dependent services such as the portal and development OTA to explicit state-machine lifecycle flow.
- Ensure services log or expose why startup is blocked when dependencies are not ready.
- Keep dependent services responsible for handling WiFi loss, backend restart, or safe continued operation.

**Non-Goals:**

- Do not add a dynamic runtime service registry.
- Do not make `App` inspect service-specific readiness and start services manually.
- Do not add MQTT in this change; document and prepare the pattern for future MQTT.
- Do not change persisted configuration format.
- Do not introduce blocking waits in setup or loop.

## Decisions

1. `App::begin()` configures services only.

   `App::begin()` will initialize platform/config storage and call service `begin(...)` methods to inject configuration and dependencies. It will not call `tick()` on runtime services and will not start network backends indirectly through begin-time service ticks.

2. WiFi readiness is represented explicitly.

   `WifiManager` or a narrow dependency interface will expose readiness signals needed by dependent services. At minimum this includes network stack initialized, station connected/addressed where required, and setup AP/address readiness where required.

   The readiness terms are:

   - `networkStackReady`: the WiFi/TCP-IP stack has been initialized successfully and WiFi is not in `WIFI_OFF`/`WIFI_MODE_NULL`/down state. In the Arduino ESP32 core, `WiFi.mode(...)` reaches the path that initializes `esp_netif`; switching WiFi off later calls the stop/deinit path and invalidates this dependency.
   - `stationReady`: station mode is connected and the station IP address is valid/non-zero. LwIP-based services such as OTA or MQTT must wait for this, not merely for association.
   - `setupApReady`: SoftAP startup succeeded and the AP IP address is valid/non-zero. `setupApActive_` must be set only after `WiFi.softAP(...)` succeeds and must be cleared on AP stop or failed AP startup.

3. WiFi-dependent services use explicit state-machine lifecycle.

   `PortalServer`, development OTA, and future network services should have states such as `Idle`, `WaitingForDependency`, `Starting`, `Running`, and `Faulted` where appropriate. The exact state names can vary, but dependency waits and backend startup must be explicit and testable.

4. Portal backend startup waits for network stack readiness.

   The HTTP server must not call `AsyncWebServer::begin()` until the WiFi/network stack dependency reports ready. Captive DNS startup remains separately gated by setup AP activity and a valid setup AP IP address.

   `AsyncWebServer` uses an `AsyncServer` listener that binds to `INADDR_ANY`, so a normal AP-to-STA or STA-to-AP mode transition does not by itself require recreating the HTTP server while the TCP/IP stack remains initialized. Existing client connections may drop when an interface goes away, but the listener can remain running. If WiFi is explicitly stopped or put into `WIFI_OFF`/`WIFI_MODE_NULL`, the portal should stop its HTTP backend and return to dependency-wait until `networkStackReady` becomes true again.

5. Development OTA startup waits for usable WiFi/IP readiness.

   ArduinoOTA must not call `ArduinoOTA.begin()` until the device has a usable WiFi address on either the station side or the setup AP side. After startup, the service should either safely continue handling OTA across interface changes if the backend supports it, or move through an explicit stop/restart state if not.

   ArduinoOTA has an explicit `end()` API that stops its UDP listener and mDNS registration. The OTA service should use it when the active WiFi/IP target is lost or changes, then wait for a new usable IP and start again.

6. Dependency failures are service-local, not `App` branches.

   If a dependency is missing, a service remains in its wait state and logs a bounded diagnostic when useful. `App` still only computes `now` once and calls service ticks in deterministic order.

7. Runtime service lifecycle names are consistent.

   Runtime services use `begin(...)` for configuration/dependency wiring, `tick(uint32_t now)` for cooperative runtime advancement, and optional `end()` for explicit shutdown. The name `loop()` is reserved for the Arduino entrypoint in `main.cpp`;
domain services and the `StateMachine` helper use `tick(...)`
            .

        Services that inherit `StateMachine` should use the inherited `StateMachine::tick(
                now)` directly when no extra work is needed.A service may override `tick(now)` only when it adds bounded pre
        / post state
    - machine policy,
    and that implementation should call `StateMachine::tick(now)` rather than a separate `loop(...)` wrapper.

        8. `esp32dev_ota` is an upload environment,
    not a separate firmware verification target.

        The `esp32dev_ota` PlatformIO environment extends `esp32dev` and
        only changes OTA upload protocol / address.Routine compile verification should build `esp32dev`;
build or use `esp32dev_ota` only when changing its upload settings or
    performing an actual OTA upload check
        .

    9. Mobile provisioning is BLE-only.

    `MobileProvisioning` must not select between BLE and WiFiProv SoftAP transports.
    The firmware already has a first-class setup AP plus HTTP portal managed by
    `WifiManager` and `PortalServer`; WiFiProv SoftAP would be a second,
    unrelated AP provisioning protocol and makes service ownership ambiguous.
    BLE provisioning should start with `WIFI_PROV_SCHEME_BLE`,
    `WIFI_PROV_SCHEME_HANDLER_NONE`, and `service_key == nullptr`.
    Repeated BLE starts should be handled by explicit state-machine cooldown after
    `wifi_prov_mgr_deinit()`, not by changing the provisioning transport.

                ##Risks /
                Trade -
            offs

            - Startup could be delayed by stricter readiness checks->Use the narrowest readiness required by each backend and cover first -
            boot / no - credentials paths in tests.- Too many service -
            specific readiness methods on `WifiManager` ->Prefer a small interface or
        focused methods named by dependency semantics,
    not by consuming service names.- AsyncWebServer restart may be unnecessary across AP /
                                         STA interface changes->Keep the HTTP listener running across AP /
                                         STA transitions while `networkStackReady` remains true; stop/restart only when the WiFi/TCP-IP dependency is explicitly lost.
- ArduinoOTA reconnect behavior may differ from HTTP portal behavior -> Keep OTA state-machine policy separate from portal state-machine policy and use `ArduinoOTA.end()` when station readiness is lost.
- More states can make service code larger -> Keep each state focused and use the existing `StateMachine` helper rather than a general framework.

## Migration Plan

1. Add WiFi readiness methods or a narrow dependency interface.
2. Fix setup AP state tracking so AP readiness is never reported after a failed `WiFi.softAP(...)` call.
3. Convert `PortalServer` begin/tick to configuration plus dependency-gated state-machine startup.
4. Convert `ArduinoOtaService` to dependency-gated state-machine startup.
5. Remove begin-time service ticks from `App::begin()`.
6. Add host tests for boot ordering and dependency-gated startup.
7. Run host tests, the base ESP32 firmware build, and manual hardware verification for first boot, provisioning, portal, station reconnect, and development OTA.

## Open Questions

- Confirm on hardware that keeping `AsyncWebServer` running across AP-to-STA and STA-to-AP transitions works as expected while the WiFi/TCP-IP stack remains initialized.
