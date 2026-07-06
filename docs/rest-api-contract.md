# REST API Contract

This document is the canonical public REST contract for the portal API. The
frontend model in `portal-spa/src/api/contracts.ts` is the TypeScript mirror of
this contract.

## Contract Rules

- Public JSON uses `camelCase`.
- Device records use the nested `record`, `config`, and `runtime` shape.
- Device creation uses `typeName`, not numeric `typeId`.
- Numeric device type ids are firmware/catalog internals and are not required in
  public REST payloads.
- Device commands use camelCase command names: `updateConfig` and `setOutput`.
- Persisted settings live in `config`; live state, statuses, scans, and outputs
  live in `runtime`.
- `registryRevision` belongs to response envelopes, not to individual device
  config objects.
- Successful JSON responses include `success: true` when a body is returned.
- Errors use `{"success":false,"code":"...","error":"..."}`.
- Routes that mutate JSON use `Content-Type: application/json`.

## Common Types

```ts
type DeviceOutputState = 'off' | 'on' | 'disabled'
type TemperatureUnit = 'celsius' | 'fahrenheit'

interface DeviceDependencyLink {
  role: string
  deviceId: number
}

interface DeviceRecordBase {
  id: number
  typeName: string
  configRevision: number
}

interface BaseDeviceConfig {
  name: string
  enabled: boolean
  deps: DeviceDependencyLink[]
}

interface BaseDeviceRuntime {
  status: string
  lifecycleStatus: string
  effectiveStatus: string
  dependencyStatus?: string
}

interface DeviceRecord<
  TConfig extends BaseDeviceConfig = BaseDeviceConfig,
  TRuntime extends BaseDeviceRuntime = BaseDeviceRuntime,
> {
  record: DeviceRecordBase
  config: TConfig
  runtime: TRuntime
}

interface TemperatureOutputSnapshot {
  value: number
  unit: TemperatureUnit
  unitSymbol: string
  measuredAtMs: number
  valid: boolean
  status?: string
}
```

Status strings currently used by device runtime snapshots include `creating`,
`starting`, `ready`, `disabled`, `faulted`, `dependency_blocked`,
`reconfiguring`, `stopping`, `deleting`, and `unknown`.

## Device Types

Supported public `typeName` values:

- `dummy`
- `gpio_switch`
- `onewire_bus`
- `ds18b20_temperature_sensor`
- `thermostat`

## Device Config And Runtime

### Dummy

```ts
interface DummyConfig extends BaseDeviceConfig {}
interface DummyRuntime extends BaseDeviceRuntime {}
```

### GPIO Switch

```ts
interface GpioSwitchConfig extends BaseDeviceConfig {
  restorePreviousState: boolean
  startupState: DeviceOutputState
  safeState: DeviceOutputState
  inverted: boolean
  gpioPin: number
}

interface GpioSwitchRuntime extends BaseDeviceRuntime {
  output?: {
    state?: DeviceOutputState
    physicalLevel?: boolean
  }
}
```

### OneWire Bus

```ts
interface OneWireBusConfig extends BaseDeviceConfig {
  gpioPin: number
  internalPullup: boolean
}

interface OneWireScanDeviceSnapshot {
  address: string
  familyCode: string
}

interface OneWireScanSnapshot {
  inProgress: boolean
  ready: boolean
  deviceCount: number
  truncated: boolean
  invalidCrcSeen: boolean
  devices: OneWireScanDeviceSnapshot[]
}

interface OneWireBusRuntime extends BaseDeviceRuntime {
  scan?: OneWireScanSnapshot
}
```

### DS18B20 Temperature Sensor

```ts
interface Ds18b20Config extends BaseDeviceConfig {
  address: string
  resolution: 9 | 10 | 11 | 12
  unit: TemperatureUnit
  pollMs: number
  reportDeltaCelsius: number
  reportAlways: boolean
}

interface Ds18b20Runtime extends BaseDeviceRuntime {
  output?: {
    temperature?: TemperatureOutputSnapshot
  }
  consecutiveErrors?: number
  lastDependencyGeneration?: number
}
```

`config.deps` must contain one `onewire_bus` dependency link.

### Thermostat

```ts
interface ThermostatConfig extends BaseDeviceConfig {
  mode: 'off' | 'heat' | 'cool'
  algorithm: 'hysteresis'
  targetCelsius: number
  minSafeCelsius: number
  maxSafeCelsius: number
  hysteresisCelsius: number
  checkIntervalMs: number
  sensorTimeoutMs: number
  retryAfterErrorMs: number
  minSwitchIntervalMs: number
  temperatureSensorDeviceId?: number
  switchDeviceId?: number
}

interface ThermostatRuntime extends BaseDeviceRuntime {
  output?: {
    desiredSwitchState?: DeviceOutputState
    actualSwitchState?: DeviceOutputState
    controlStatus?: string
    lastCheckAtMs?: number
    temperature?: TemperatureOutputSnapshot
  }
}
```

`config.deps` must contain one `temperature_sensor` dependency link and one
`switch` dependency link. `temperatureSensorDeviceId` and `switchDeviceId` are
frontend convenience mirrors; `deps` is the authoritative persisted
relationship contract.

## Device Registry

### `GET /api/devices`

Returns the complete device registry snapshot.

```ts
interface DeviceRegistryResponse {
  success: true
  registryRevision: number
  devices: DeviceRecord[]
}
```

### `GET /api/devices/:id`

Returns a single device snapshot.

```ts
interface DeviceDetailResponse {
  success: true
  registryRevision: number
  device: DeviceRecord
}
```

### `POST /api/devices`

Creates a device. The request must contain the public `typeName` and a complete
persisted `config` object for that device type.

```ts
interface DeviceCreateRequest<TConfig extends BaseDeviceConfig = BaseDeviceConfig> {
  typeName: string
  config: TConfig
}
```

Example:

```json
{
  "typeName": "gpio_switch",
  "config": {
    "name": "GPIO Relay",
    "enabled": true,
    "deps": [],
    "restorePreviousState": false,
    "startupState": "off",
    "safeState": "disabled",
    "inverted": false,
    "gpioPin": 4
  }
}
```

Response:

```ts
interface DeviceMutationResponse {
  success: true
  registryRevision: number
  device?: DeviceRecord
}
```

### `POST /api/devices/:id/command`

Executes a structured command against a device.

```ts
interface DeviceCommandRequest {
  deviceId?: number
  command: 'delete' | 'updateConfig' | 'scan' | 'setOutput'
  state?: DeviceOutputState
  config?: Record<string, unknown>
  deps?: DeviceDependencyLink[]
}
```

Command rules:

- `delete` and `scan` require no extra field.
- `setOutput` requires `state`.
- `updateConfig` requires `config`. `config` may be a partial object containing
  only the keys that changed — the device applies a partial merge against its
  current persisted config, so any field omitted from `config` keeps its
  current value (omitted fields are never reset to compiled defaults). This
  includes `name` and `enabled`: there is no separate rename/enable/disable
  command — renaming a device or toggling it on/off is just another
  `updateConfig` field change. Renaming rejects with `INVALID_CONFIG` if
  another device already has that name; flipping `enabled` drives the
  device's lifecycle (start/stop) the same way a dedicated command used to.
  `deps`, when included, is not merged: it must be the full current
  dependency snapshot for the device (all roles), matching existing
  behavior.
- `deviceId`, when provided, must match the `:id` path parameter.
- Public clients must not send packed `payload` strings or binary config blobs.

Examples:

```json
{ "command": "setOutput", "state": "on" }
```

```json
{
  "command": "updateConfig",
  "config": {
    "name": "Water Temperature",
    "enabled": true,
    "deps": [{ "role": "onewire_bus", "deviceId": 670845751 }],
    "address": "28FF641D621603AD",
    "resolution": 12,
    "unit": "celsius",
    "pollMs": 5000,
    "reportDeltaCelsius": 0.25,
    "reportAlways": false
  },
  "deps": [{ "role": "onewire_bus", "deviceId": 670845751 }]
}
```

Partial update — only `reportDeltaCelsius` changed; all other fields
(`address`, `resolution`, `unit`, `pollMs`, `reportAlways`, `name`, `enabled`)
are left untouched server-side:

```json
{
  "command": "updateConfig",
  "config": { "reportDeltaCelsius": 0.5 }
}
```

Rename only (rejects with `INVALID_CONFIG` if another device already has this name):

```json
{
  "command": "updateConfig",
  "config": { "name": "Water Temperature (kitchen)" }
}
```

Disable/enable only (drives the device's start/stop lifecycle the same way a
dedicated command used to):

```json
{
  "command": "updateConfig",
  "config": { "enabled": false }
}
```

### `DELETE /api/devices/:id`

Deletes a device. On dependency conflicts, the error body includes
`dependentDeviceIds`.

### `POST /api/devices/flush`

Forces pending registry persistence to be flushed.

```ts
interface DeviceFlushResponse {
  success: true
  registryRevision: number
}
```

## WiFi

### `GET /api/wifi/status`

```ts
interface WifiStatusResponse {
  success: true
  wifiStatus: 'connected' | 'connecting' | 'disconnected' | 'failed' | 'idle' | 'ble_config'
  stationIp: string
  setupApIp: string
}
```

### `GET /api/wifi/scan`

Returns `202` while scanning and `200` when results are ready.

```ts
interface WifiScanNetwork {
  ssid: string
  rssi: number
  channel: number
}

interface WifiScanResponse {
  success?: true
  status: 'ok' | 'scanning'
  networks?: WifiScanNetwork[]
}
```

### `POST /api/wifi/configure`

```json
{ "ssid": "Network", "password": "optional" }
```

Accepted response:

```json
{ "success": true, "status": "accepted" }
```

### `DELETE /api/wifi/configure`

Clears stored WiFi credentials.

```json
{ "success": true, "status": "accepted", "action": "clear_wifi_credentials" }
```

### `POST /api/wifi/ble-config`

Starts BLE WiFi provisioning when available.

```json
{ "success": true, "status": "accepted", "action": "start_ble_config" }
```

## Dashboard Layout

### `GET /api/dashboard/layout`

```ts
type DashboardLayoutWidgetRecord = [deviceId: number, x: number, y: number, w: number, h: number]

interface DashboardLayoutPanelRecord {
  id: string
  name: string
  order: number
  widgets: DashboardLayoutWidgetRecord[]
}

interface DashboardLayoutRecord {
  schemaVersion: number
  activePanelId: string
  panels: DashboardLayoutPanelRecord[]
}

interface DashboardLayoutResponse {
  success: true
  revision: number
  layoutDefaulted?: boolean
  layout: DashboardLayoutRecord
}
```

### `PUT /api/dashboard/layout`

Accepts either a raw `DashboardLayoutRecord` or `{ "layout": DashboardLayoutRecord }`.
Returns `204 No Content` on success.

## OTA

### `GET /api/ota/status`

```ts
interface OtaStatusResponse {
  success: true
  enabled: boolean
  freeSketchSpace: number
  hasError: boolean
}
```

### `POST /api/ota`

Uploads firmware through the request body. On success, the registry is flushed,
the controller returns a closing response, and the device reboots.

```json
{ "success": true, "status": "ok", "rebooting": true }
```

## MQTT

Optional feature, compiled in only with `-DWITH_HOME_ASSISTANT` (see
`docs/mqtt-home-assistant.md`). `enabled` in `MqttStatusResponse` reflects
whether the flag was compiled in, not the runtime on/off toggle (that's
`MqttSettingsRecord.enabled`) — the frontend uses the former to gray out the
settings page and the latter to drive the enable switch.

### `GET /api/mqtt/status`

```ts
interface MqttStatusResponse {
  success: true
  enabled: boolean // compiled in (WITH_HOME_ASSISTANT)
  connected: boolean
  waitingForStation: boolean
  host: string
  port: number
  useTls: boolean
  clientId: string
  hasCaCert: boolean
}
```

### `GET /api/mqtt/settings` / `PUT /api/mqtt/settings`

`PUT` accepts a partial object — only provided fields are updated. `haNodeId`
defaults (once, on first boot) to `<deviceName>-<macSuffix>` sanitized to
`[a-zA-Z0-9_-]`; changing it after devices have been announced to Home
Assistant creates a **new** device there (old entities/history are orphaned).

```ts
interface MqttSettingsRecord {
  enabled: boolean
  host: string
  port: number
  useTls: boolean
  clientId: string
  username: string
  password: string // write-only; GET responses omit it (see passwordRedacted)
  passwordRedacted: boolean // GET only
  haDiscoveryPrefix: string
  haNodeId: string
  haNodeName: string
  hasCaCert: boolean
}
```

### `POST /api/mqtt/ca-cert` / `DELETE /api/mqtt/ca-cert`

Multipart upload (field name `cert`) of a PEM-encoded CA certificate, used
only when `useTls` is `true`. `DELETE` clears the stored certificate. Both
respond with the same shape as `GET /api/mqtt/status`.

### Per-device Home Assistant opt-in

Devices are **not** published to Home Assistant by default. Each device's
JSON envelope gains a generic `ha` block (present only when
`WITH_HOME_ASSISTANT` is compiled in), alongside `record`/`config`/`runtime`:

```ts
interface DeviceHaSettings {
  enabled: boolean
  name: string // override; empty means "use config.name"
  effectiveName: string // name override if set, else config.name
}
```

Toggling it is a `cmd` on the device (`POST /api/devices/:id`, mirroring
`setOutput`/`setDeps`):

```json
{ "command": "setHaSettings", "haEnabled": true, "haName": "Pump" }
```

## System

### `POST /api/system/restart`

Flushes the registry and schedules a controller restart.

```ts
interface SystemRestartResponse {
  success: true
  rebooting: boolean
}
```

## Device Setup Transfer

### `GET /api/device-setup/export`

Returns `application/x-ndjson`. Each device entry contains setup data only:
identity and persisted `config`; runtime state is omitted.

### `POST /api/device-setup/import`

Accepts a multipart upload field named `bundle` containing the exported NDJSON
bundle.

```ts
interface DeviceSetupTransferResponse {
  success: true
  registryRevision: number
  deviceCount: number
}
```

## Error Codes

Known error codes include `BAD_ARGS`, `BAD_PARAMS`, `BAD_JSON`, `BOUNDS_EXCEEDED`,
`BUSY`, `DEPENDENT_DELETE`, `DUPLICATE_DEVICE_ID`, `INTERNAL`,
`INVALID_COMMAND`, `INVALID_CONFIG`, `INVALID_DEVICE_ID`,
`INVALID_RELATIONSHIP`, `INVALID_VERSION`, `NOT_FOUND`, `OTA_FAILED`,
`STORAGE_ERROR`, and `UNSUPPORTED_TYPE`.

MQTT-specific codes (`## MQTT`): `HOST_REQUIRED`, `HOST_TOO_LONG`,
`PORT_INVALID`, `CLIENT_ID_REQUIRED`, `CLIENT_ID_TOO_LONG`,
`USERNAME_TOO_LONG`, `PASSWORD_TOO_LONG`, `DISCOVERY_PREFIX_INVALID`,
`NODE_ID_REQUIRED`, `NODE_ID_TOO_LONG`, `NODE_ID_INVALID_CHARACTERS`,
`NODE_NAME_TOO_LONG`, and `CERT_TOO_LARGE`.

Error example:

```json
{
  "success": false,
  "code": "BAD_ARGS",
  "error": "name is required"
}
```

## Compatibility Notes

Legacy snake_case command names such as `update_config`, `set_status`, and
`set_output` are not part of the public REST contract. Backend code may accept
them temporarily during migration, but frontend code and tests should use the
camelCase command names documented here.

Legacy public create payloads using `type`, `typeId`, or `type_id` are not part
of this contract. Clients must use `typeName`.
