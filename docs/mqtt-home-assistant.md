# MQTT + Home Assistant Integration

Optional feature that connects the firmware to an MQTT broker and publishes
Home Assistant MQTT discovery for supported devices. Disabled by default.

## Enabling it

Uncomment in `platformio.ini`:

```ini
	-DWITH_HOME_ASSISTANT
```

This pulls in `knolleary/PubSubClient` and `WiFiClientSecure` (see
`docs/platformio-environments.md`'s flash budget note before enabling it on a
4 MB board).

### Effect of `WITH_HOME_ASSISTANT` being undefined

- `/api/mqtt/*` routes are never registered on the portal HTTP server (any
  request to them 404s) and the per-device `ha` JSON block / `setHaSettings`
  command are absent from the device registry API.
- No MQTT/HA code is linked into the firmware at all — smaller flash, no
  `PubSubClient`/`WiFiClientSecure` dependency.
- The `mqtt.status` WebSocket topic still gets pushed, but always reports
  `enabled: false, connected: false, waitingForStation: false`.
- The portal UI's "Home Assistant" page renders only an explanatory alert
  instead of the settings form, and the per-device "Home Assistant" card on
  the device detail page doesn't render at all.

### Two different meanings of "enabled" — don't confuse them

This feature has **two independent on/off states** that are easy to mix up:

1. **Compiled in** (`WITH_HOME_ASSISTANT`, surfaced as
   `MqttStatusResponse.enabled` / `MqttController::compiledIn()`) — whether
   this firmware *build* has the feature at all. The portal UI's top-level
   "Available"/"Not available" chip on the Home Assistant page reflects
   *this*, not whether MQTT is actively connecting.
2. **Runtime toggle** (`MqttSettings.enabled`, the "Enable MQTT" switch in the
   settings form, persisted via `MqttConfigStore`) — whether the firmware
   should actually attempt to connect to a broker right now. A firmware
   compiled with the flag can still show "Available" while MQTT itself is
   switched off and nothing is being published.

Earlier UI copy reused the generic `status.enabled`/`status.disabled` labels
for the compile-time chip, which reads as if it meant the runtime toggle —
it now uses dedicated `mqtt.compiledInAvailable`/`mqtt.compiledInUnavailable`
strings instead, precisely to avoid this collision.

## Architecture

- `src/platform/MqttManager` — a `StateMachine`-based transport (mirrors
  `ArduinoOtaService`). Gated on `WifiManager::stationReady()` — MQTT never
  attempts to connect in AP-fallback mode, only once the device has a real
  station IP. Runtime settings changes (host, TLS, credentials) bump an
  internal revision counter that forces a clean reconnect on the next tick,
  without a reboot. `onConnect()`/`onMessage()`/`onDisconnect()` are
  **multi-subscriber** (each holds a `std::vector` of callbacks, not a single
  one) — `HaDiscoveryBridge` and `SystemHaPublisher` (see below) each
  register independently without overwriting each other's callback.
- `src/integrations/mqtt/HaDiscoveryBridge` — a `IDeviceEventSink` that
  bridges `DeviceRegistry` changes to Home Assistant MQTT discovery. Knows
  nothing about specific device types; it only talks to the
  `IHaEntityAdapter` interface (`src/integrations/mqtt/HaEntityAdapter.h`).
  Adding a new publishable device type means adding one more adapter, not
  touching the bridge. Convention: every adapter's discovery payload sets an
  explicit `icon` (an `mdi:*` slug) — don't rely on Home Assistant's
  per-domain/per-`device_class` default icon.
  - Each adapter requests as many topics as its entity needs via a
    `HaTopicBuilder` callback (`topicFor(channel, suffix)` →
    `<haNodeId>/<channel>/<deviceId>/<suffix>`) instead of the bridge handing
    it one fixed state/command topic pair. A simple entity (switch, sensor)
    uses its own `haComponent()` as the channel; a multi-topic entity
    (thermostat) uses distinct channel names per topic — see "Topic scheme"
    below.
  - Applying an incoming command is fully owned by the adapter too:
    `IHaEntityAdapter::applyCommand(registry, runtime, deviceId, commandKey,
    payload, now)` mutates the device however that type requires — a
    lightweight `DeviceCommand` via `DeviceRegistry::command()` for simple
    runtime state (switch ON/OFF), or a full validated config replace via
    `DeviceRegistry::updateConfig()` for persisted settings (thermostat
    mode/setpoint) — the bridge never needs to know which.
- `src/integrations/mqtt/gpio_switch/GpioSwitchHaEntityAdapter` — the
  GPIO-switch-specific adapter: builds the HA `switch` discovery payload, the
  ON/OFF state payload, and applies incoming `ON`/`OFF` commands as the same
  `DeviceCommand{SetOutput, ...}` shape the REST API already uses (so HA
  commands go through the same validation/persistence path as the portal UI).
- `src/integrations/mqtt/ds18b20/Ds18b20HaEntityAdapter` — the
  DS18B20-specific adapter: builds the HA `sensor` discovery payload
  (`device_class: "temperature"`, `unit_of_measurement: "°C"`) and the state
  payload from the device's latest temperature reading. Read-only —
  `applyCommand()` always rejects, and no `command_topic` is published.
- `src/integrations/mqtt/thermostat/ThermostatHaEntityAdapter` — the
  thermostat-specific adapter: builds the HA `climate` discovery payload
  (`modes: ["off","heat","cool"]`, `min_temp`/`max_temp` from the thermostat's
  own safe range, `temperature_unit: "C"`) and publishes four independent
  state messages (mode, setpoint, current temperature, `action`). Unlike the
  other two adapters, its `applyCommand()` doesn't call
  `DeviceRegistry::command()` at all: `ThermostatDevice` has
  `supportsCommands = false` and no `handleCommand()` override — mode and
  setpoint are persisted config fields, so an incoming HA command is patched
  onto a copy of the current `ThermostatDeviceConfigV1`, re-encoded, and
  applied via `DeviceRegistry::updateConfig()` — the exact same
  validate-then-persist path the portal's own "update config" REST call
  uses, so an out-of-range setpoint from Home Assistant is rejected the same
  way an out-of-range setpoint from the portal UI would be.
- `src/config/MqttConfigStore` / `src/config/MqttConfig.h` — scalar settings
  persisted the same way as WiFi credentials (`ConfigStore`); the CA
  certificate is a variable-length blob stored via raw `putBlob`/`getBlob`
  (same primitive `DisplayLayoutStore` uses for its layout blob), since it
  doesn't fit the fixed 512-byte per-device config-blob pattern.
- `src/integrations/mqtt/HaDeviceSettings` — per-device HA opt-in
  (`enabled`, optional name override), stored via the existing
  `DeviceScopedDataStore` (the same per-device NVS mechanism
  `DisplayLayoutStore` uses), completely independent of each device type's
  own config struct. A device is **not** published to Home Assistant until
  the user explicitly opts in (`setHaSettings` command, `cmd` action on
  `POST /api/devices/:id`).
- `src/integrations/mqtt/system/SystemHaPublisher` — publishes firmware-level
  diagnostics (uptime, free heap, heap fragmentation, WiFi RSSI/SSID/IP) and
  a restart button. Deliberately **not** an `IHaEntityAdapter` and not tied
  to `DeviceRegistry` at all — these entities describe the board itself, not
  any device in the registry, so they're always published (no per-entity
  opt-in) whenever MQTT is enabled and connected, under the same HA `device`
  block as the per-device entities. Discovery + a state refresh happen once
  on every MQTT connect (birth), then state alone repeats every 30s via
  `tick(now)` (called from `App::tick()`, same as `MqttManager::tick()`).
  The restart button reuses `SystemRestartController` — the exact same
  flush-before-reboot safety check and `esp_timer`-based reboot scheduling
  the REST `POST /api/system/restart` endpoint uses (extracted so both paths
  share one implementation instead of duplicating it).

## Command & state round-trip

A command can arrive from two places — Home Assistant over MQTT, or a person
in the portal UI over REST — and both are handled identically from
`DeviceRegistry::command()` onward. Whenever a device's state actually
changes, one event fans back out to **both** destinations at once, so the
portal and Home Assistant never drift apart.

```mermaid
flowchart TD
    HA["Home Assistant<br/>publishes to<br/>&lt;haNodeId&gt;/switch/&lt;id&gt;/set<br/>payload &quot;ON&quot; / &quot;OFF&quot;"]:::ha
    UI["Portal UI<br/>POST /api/devices/:id<br/>{ command: setOutput, state: on }"]:::ui

    HA --> MM["MqttManager<br/>PubSubClient callback<br/>dispatchIncoming()"]:::ha
    UI --> DRC["DeviceRegistryController::cmd()<br/>parses body, builds DeviceCommand"]:::ui

    MM --> HDB["HaDiscoveryBridge::onMqttMessage()<br/>finds device + adapter, then<br/>GpioSwitchHaEntityAdapter::applyCommand()"]:::ha

    HDB --> DR["DeviceRegistry::command(cmd, now)<br/>— single entry point —"]:::junction
    DRC --> DR

    DR --> RUNTIME["Device runtime mutates<br/>GpioSwitchDevice / SwitchDeviceBase::handleCommand()<br/>→ physical GPIO output changes"]:::neutral
    RUNTIME --> EVT["DeviceRegistryEventReporter::emit()<br/>DeviceEvent{ kind: StateChanged, deviceId, ... }"]:::neutral
    EVT --> BUS["DeviceEventDispatcher → DeviceEventBus::publish()<br/>— single fan-out point —<br/>delivered to every registered IDeviceEventSink"]:::junction

    BUS --> HDB2["HaDiscoveryBridge::onDeviceEvent()<br/>StateChanged → publishStateOnly()"]:::ha
    BUS --> WS["PortalWebSocketManager::onDeviceEvent()<br/>builds device.upsert / command_result"]:::ui

    HDB2 --> MQTTOUT["GpioSwitchHaEntityAdapter::publishState()<br/>→ MqttManager::publish(state topic, retain=true)"]:::ha
    WS --> WSOUT["PortalWebSocketManager::sendText()<br/>→ socket_->textAll()"]:::ui

    MQTTOUT --> HAOUT["MQTT broker → Home Assistant<br/>entity shows the new state"]:::ha
    WSOUT --> UIOUT["Every open portal tab<br/>updates live, no refresh needed"]:::ui

    classDef ha fill:#e6f3f1,stroke:#1c7c78,color:#0f4d4a;
    classDef ui fill:#ecedf8,stroke:#4750a6,color:#2e3576;
    classDef junction fill:#fbf1de,stroke:#b9822f,color:#5c3f13,stroke-width:2px;
    classDef neutral fill:#eef1f0,stroke:#c3cbc9,color:#16211f;
```

**Why two junctions matter:** both sinks — `HaDiscoveryBridge` and
`PortalWebSocketManager` — register on the same `DeviceEventDispatcher` once,
via `attachDispatcher()`, at startup. Neither class knows the other exists. A
change made from Home Assistant reaches the browser, and a change made in the
browser reaches Home Assistant, purely because they both listen to the same
event bus — not because either side calls the other directly.

This diagram uses the GPIO switch as the worked example. The thermostat's
`ThermostatHaEntityAdapter::applyCommand()` sits at the same "DR" junction
but calls `DeviceRegistry::updateConfig()` instead of `::command()` — see
"Architecture" above for why (mode/setpoint are persisted config, not
lightweight runtime state). Everything downstream of that junction (the
event bus fan-out to both sinks) is identical either way.

## Node and entity identifiers

- **`haNodeId`** — stable identifier used both as the MQTT topic root
  (`<haNodeId>/status`, `<haNodeId>/switch/<deviceId>/state|set`) and as the
  Home Assistant `device.identifiers` value. Defaults, on first boot, to
  `<deviceName>-<macSuffix>` (the same formula `WifiManager::setupApSsid()`
  uses for the setup AP SSID), sanitized to `[a-zA-Z0-9_-]` — the character
  class Home Assistant's MQTT discovery topic scheme requires. **Changing it
  after devices have been announced creates a new device in Home Assistant**
  (the old one is orphaned); the settings UI warns about this.
- **`haNodeName`** — free-form display name for the Home Assistant device
  (`device.name`). Safe to rename at any time; has no effect on topics or
  identifiers.
- Each published device gets `unique_id = "<haNodeId>_<typeName>_<deviceId>"`
  (globally unique across multiple boards sharing one broker/HA instance —
  `deviceId` alone is only unique within one board's registry) and
  `has_entity_name: true` with `name` = the per-device HA name override if
  set, else the device's own `config.name`. `object_id`/`entity_id` are left
  for Home Assistant to auto-generate; this project doesn't attempt to
  control them via `default_entity_id`.
- Every discovery payload includes a shared `device` block (grouping all
  entities from one board into one Home Assistant device) and an `origin`
  block (`{name: "ESP32WIFIManager"}`) — the latter is **required** by Home
  Assistant's MQTT discovery schema whenever a payload includes a `device`
  block.

## Topic scheme

```
<haDiscoveryPrefix>/switch/<haNodeId>/<haNodeId>_gpio_switch_<deviceId>/config   discovery (retained)
<haNodeId>/status                                                               availability (retained, LWT)
<haNodeId>/switch/<deviceId>/state                                              state (retained, "ON"/"OFF")
<haNodeId>/switch/<deviceId>/set                                                command ("ON"/"OFF")

<haDiscoveryPrefix>/sensor/<haNodeId>/<haNodeId>_ds18b20_temperature_sensor_<deviceId>/config   discovery (retained)
<haNodeId>/sensor/<deviceId>/state                                              state (retained, e.g. "23.46")

<haDiscoveryPrefix>/climate/<haNodeId>/<haNodeId>_thermostat_<deviceId>/config   discovery (retained)
<haNodeId>/climate_mode/<deviceId>/state                                        mode state (retained, "off"/"heat"/"cool")
<haNodeId>/climate_mode/<deviceId>/set                                          mode command ("off"/"heat"/"cool")
<haNodeId>/climate_temperature/<deviceId>/state                                 setpoint state (retained, e.g. "24.50")
<haNodeId>/climate_temperature/<deviceId>/set                                   setpoint command (e.g. "24.5")
<haNodeId>/climate_current_temperature/<deviceId>/state                        current temperature (retained, e.g. "23.80")
<haNodeId>/climate_action/<deviceId>/state                                      hvac action (retained, "off"/"idle"/"heating"/"cooling")

# each system sensor has its own <haDiscoveryPrefix>/sensor/<haNodeId>/<haNodeId>_system_<key>/config
<haNodeId>/system/uptime/state                                                  seconds since boot (retained, e.g. "3600")
<haNodeId>/system/free_heap/state                                               bytes (retained, e.g. "123456")
<haNodeId>/system/heap_fragmentation/state                                      percent 0-100 (retained, e.g. "12")
<haNodeId>/system/wifi_rssi/state                                               dBm (retained, e.g. "-55")
<haNodeId>/system/wifi_ssid/state                                               currently associated SSID (retained)
<haNodeId>/system/wifi_ip/state                                                 station IP (retained)

<haDiscoveryPrefix>/button/<haNodeId>/<haNodeId>_system_restart/config           discovery (retained)
<haNodeId>/system/restart/set                                                   command (any payload triggers it)
```

DS18B20 sensors have no `set` topic — the wildcard command subscription below
still matches their state topic shape, but `Ds18b20HaEntityAdapter::applyCommand()`
always rejects, so no command ever reaches the device.

The thermostat's "channel" segment (`climate_mode`, `climate_temperature`,
`climate_current_temperature`, `climate_action`) is not the HA component name
(that's always `climate`, used only in the discovery topic) — it's a
per-topic key `ThermostatHaEntityAdapter` picks so a single entity can own
several independent state/command topics. `HaDiscoveryBridge` never
special-cases this; it just passes whatever channel the topic used through
to `applyCommand()`/`publishState()` as an opaque string.

The bridge subscribes to a **single wildcard** `<haNodeId>/+/+/set` once, on
connect — not one subscription per entity or per channel. New devices (and
new channels within an existing entity) are picked up automatically without
any resubscribe; incoming messages are routed to the right device by parsing
the topic's device-id segment and looking up its runtime/adapter, not by the
number of active subscriptions.

`SystemHaPublisher`'s `system/restart/set` topic also matches this same
wildcard shape (`<haNodeId>/system/restart/set` → 4 segments), so both
`HaDiscoveryBridge` and `SystemHaPublisher` receive every incoming message
(that's why `MqttManager::onMessage()` is multi-subscriber) and each just
ignores whatever isn't meant for it — `HaDiscoveryBridge` fails to parse
`"restart"` as a numeric device id and returns; `SystemHaPublisher` only
reacts to the one exact topic it owns.

Unlike per-device entities, the system entities have **no opt-in toggle** -
they're always published whenever MQTT is enabled and connected, since they
describe the board itself rather than a specific `DeviceRegistry` device.
They're also on a different cadence: instead of being event-driven
(published only when something changes), state repeats unconditionally every
30 seconds, because "free heap" and "uptime" don't have discrete change
events the way a switch's on/off state does.

## Lifecycle (birth/LWT)

- `MqttManager::setWill(<haNodeId>/status, "offline", retain=true)` is armed
  once, before the first connection attempt, so the broker publishes it if
  the device disconnects uncleanly.
- On every successful connect, `HaDiscoveryBridge` publishes `"online"` to
  `<haNodeId>/status` (retained), resubscribes the command wildcard, and
  republishes discovery + current state for every device that has opted in —
  this is what makes a broker restart or firmware reconnect self-healing
  without manual re-discovery in Home Assistant. `SystemHaPublisher`
  independently does the same for the system diagnostics/restart entities on
  the same connect event (both register their own `MqttManager::onConnect()`
  callback).

## TLS

- CA-certificate-only server verification (`WiFiClientSecure::setCACert`), no
  client certificate/mTLS.
- If `useTls` is enabled but no CA certificate has been uploaded,
  `MqttManager` refuses to connect (goes to a `Faulted` state with a log
  line) rather than silently falling back to an insecure connection.
- The certificate is uploaded as a PEM file via
  `POST /api/mqtt/ca-cert` (multipart, field `cert`) and removed via
  `DELETE /api/mqtt/ca-cert`.

## REST contract

See `docs/rest-api-contract.md`'s `## MQTT` section for the full request/response
shapes (`/api/mqtt/status`, `/api/mqtt/settings`, `/api/mqtt/ca-cert`, and the
per-device `ha` block / `setHaSettings` command).

## Current scope

Three device types are published to Home Assistant in this iteration:

- GPIO switch (`gpio_switch`) — mapped to HA's `switch` component, with
  `icon: "mdi:toggle-switch"` and command support (`ON`/`OFF` routed back
  through `DeviceRegistry::command`).
- DS18B20 temperature sensor (`ds18b20_temperature_sensor`) — mapped to HA's
  `sensor` component with `device_class: "temperature"`,
  `state_class: "measurement"`, `unit_of_measurement: "°C"`, and
  `icon: "mdi:thermometer"`. Read-only: no `command_topic` is published and
  incoming commands are rejected. The state is always published in Celsius
  regardless of the device's configured display unit (that setting only
  affects the portal UI/OLED output) — Home Assistant converts to the user's
  preferred unit on its own.
- Thermostat (`thermostat`) — mapped to HA's `climate` component, with
  `icon: "mdi:thermostat"`, `modes: ["off","heat","cool"]`,
  `min_temp`/`max_temp` taken from the thermostat's own configured safe
  range, `temp_step: 0.5`, `precision: 0.1`, `temperature_unit: "C"`. Mode
  and setpoint changes from Home Assistant are applied through
  `DeviceRegistry::updateConfig()` (full validated config replace, see
  "Architecture" above) — an out-of-range setpoint is rejected exactly like
  it would be from the portal UI. The `action` topic (`off`/`idle`/`heating`/
  `cooling`) is derived from the thermostat's own mode + actual switch output
  state, not a separate stored field.

Other device types (displays) are out of scope for now, but the
adapter/registry seam (`IHaEntityAdapter`/`HaEntityAdapterRegistry`) is
designed so adding one is a new adapter file, not a change to
`HaDiscoveryBridge` or `MqttManager`.

In addition to per-device entities, `SystemHaPublisher` always publishes six
firmware-level diagnostic sensors and a restart button, regardless of which
(if any) devices exist in the registry:

- **Uptime** — `sensor`, `device_class: "duration"`, `unit_of_measurement: "s"`,
  seconds since boot. No wall-clock/NTP dependency (the firmware doesn't sync
  real time anywhere) — a `device_class: "timestamp"` "last boot" sensor was
  considered and rejected for exactly that reason.
- **Free heap** / **heap fragmentation** — `sensor`, `entity_category: "diagnostic"`,
  backed by a new `ISystemStats` interface (`ArduinoSystemStats` wraps
  `ESP.getFreeHeap()`/`ESP.getMaxAllocHeap()`).
- **WiFi RSSI** — `sensor`, `device_class: "signal_strength"`, `unit_of_measurement: "dBm"`.
- **WiFi SSID** / **WiFi IP** — plain diagnostic `sensor`s.
- **Restart** — `button`, `device_class: "restart"`, `entity_category: "config"`.
  Routes through `SystemRestartController::requestRestart()` +
  `::scheduleReboot()` — the same flush-before-reboot safety check the REST
  restart endpoint uses, not a separate/weaker path.

WiFi RSSI/SSID needed two new non-pure `IWifiDriver` methods (`rssi()`/
`ssid()`, default `0`/`""`) — non-pure specifically so the many existing
`FakeWifiDriver` test doubles across the test suite didn't all need updating.
