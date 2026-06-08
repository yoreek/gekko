## Context

The firmware already has a controller-level `DeviceConfig` persisted through `ConfigStore` and `IConfigStorage`, a cooperative `App::tick()` boundary that computes the loop timestamp once, and domain services that can use `StateMachine` for explicit multi-step flows. Dynamic devices need a separate model because they represent user-created runtime entities such as sensors, switches, OneWire buses, I2C buses, and synthetic test devices, not the controller identity itself.

The first implementation should prove the architecture with `DummyDevice` before binding the registry to real hardware drivers or external transports. This keeps NVS persistence, lifecycle transitions, validation rules, relationship behavior, and integration events testable on the native Unity environment.

## Goals / Non-Goals

**Goals:**

- Store dynamic devices in a versioned NVS-backed registry and restore them into memory on boot.
- Give every device a stable 32-bit `DeviceId`, type, display name, enabled flag, configuration revision, lifecycle status, and optional parent relationship.
- Support type-specific binary config layout migrations without modifying old persisted config structs after release.
- Persist frequently changing restore values such as switch last state separately from device configuration.
- Support operation-level immediate, delayed, and coalesced persistence policies for registry/config/state changes.
- Allow non-unique names while keeping `DeviceId` as the stable local reference for UI, integrations, relationships, and commands.
- Validate create, update, relationship, delete, enable, disable, and command operations through one service used by Web UI and integrations.
- Propagate parent dependency status to children without mutating the child configuration that is persisted in NVS.
- Publish normalized device events so MQTT, WebSocket, Home Assistant, and future integrations can be added as transport adapters.
- Keep runtime work cooperative and use App-scheduled tick cadences such as fast-loop, 100 ms, and 1 s ticks with explicit state machines for devices or integrations that need delayed/retry behavior.
- Implement `DummyDevice` as the first device type to exercise the complete registry and event flow without real hardware.

**Non-Goals:**

- Implement real OneWire, DS18B20, I2C, switch, MQTT, WebSocket, or Home Assistant transports in the first slice.
- Add cloud fleet management, authentication policy, or remote access security beyond preserving a common command-validation boundary.
- Support unbounded numbers of devices or arbitrary JSON payload sizes in NVS.
- Make display names unique or use names as stable identifiers.
- Cascade-delete child devices by default.
- Use wall-clock timestamps as the authoritative ordering or migration mechanism.

## Decisions

1. Use a separate `DeviceRegistry` domain from controller `DeviceConfig`.

   `DeviceConfig` remains the controller-level boot/provisioning configuration. Dynamic device records live under a device-registry module and storage namespace. This avoids coupling WiFi provisioning and controller identity migrations to user-created sensor/bus records. The alternative of extending `DeviceConfig` with dynamic devices is simpler initially, but it would make boot-critical config larger, harder to validate, and harder to migrate independently.

2. Store persistent records as bounded descriptors and rebuild runtime instances on boot.

   A persistent `DeviceRecord` should contain `DeviceId`, type, name, enabled flag, record/header version, type-specific `configVersion`, `configRevision`, optional parent `DeviceId`, relationship role, and type-specific config payload. Runtime objects are created from records by a `DeviceFactory` registry after load. Runtime-only values such as current status detail, last reading, transient fault text, retry deadlines, and integration delivery state are not the source of truth in NVS. The alternative of persisting live device objects would mix hardware state with configuration and make migrations brittle.

3. Keep the NVS format versioned and bounded.

   Device registry storage should use an index plus per-device records in a separate namespace such as `devices`: a version key, an index record containing count and `{DeviceId, type}` entries, and one bounded payload record per device using a short deterministic key derived from `DeviceId`. This follows the useful shape from the old Gekko `EntityStore` and avoids rewriting one large registry blob for every device change. The registry MUST validate max device count, max name length, max record size, max index size, `DeviceId` validity, type support, and relationship consistency before saving. Hot-path telemetry MUST NOT rewrite NVS. Controller-level config does not need to use this format; existing typed config keys remain appropriate for boot/provisioning settings.

4. Use operation-level persistence policies and dirty flush queues.

   Registry mutations should update the in-memory registry immediately after validation so subsequent reads reflect accepted changes. Durability is controlled by an operation-level policy. Immediate operations write required records before returning success; these fit create/delete, relationship changes, and other mutations where reboot loss would confuse registry shape. Delayed operations update memory and mark dirty records for a later cadence flush; these fit rename, ordinary config tuning, and possibly enable/disable if that reboot-loss tradeoff is acceptable. Coalesced operations keep only the latest dirty value and flush after debounce or max-delay; these fit retained state such as switch last output. The registry should track `dirtyIndex`, dirty config record IDs, dirty retained-state IDs, first-dirty time, and last-change time. A forced `flushNow()` path should be available before controlled reboot, OTA restart, factory reset, or an explicit save/apply action. API responses and events should expose pending persistence where relevant.

5. Separate config layout version, config revision, registry revision, and optional time metadata.

   `configVersion` identifies the binary layout of the type-specific payload. `configRevision` is a monotonic per-device counter that increments when a validated configuration mutation is accepted into the in-memory registry. `registryRevision` is a monotonic registry-level counter that increments when a validated registry mutation is accepted into the in-memory registry. Delayed persistence later clears pending dirty state without incrementing these counters again. `updatedAt` or similar wall-clock metadata can be added later as best-effort information only when time is known to be valid, but it MUST NOT be used for migration, ordering, or correctness because ESP32 wall-clock time can be absent, stale, or synchronized after boot. The alternative of using `updatedAt` as the primary ordering mechanism is weaker than counters in firmware.

6. Keep old binary config layouts immutable and migrate per device type.

   Each device type should own its persisted config versions, for example `DummyConfigV1`, `DummyConfigV2`, `GpioSwitchConfigV1`, and `GpioSwitchConfigV2`. After a version is released, its struct layout should not be changed. The type descriptor loads old payloads by `configVersion`, migrates them into the current config struct, validates the result, and marks the record for rewrite in the current version after a successful migration. This localizes migrations to the device type that changed; changing DS18B20 config does not require rewriting GPIO switch migration logic, registry index logic, or controller config migration logic. Persisted binary structs should use fixed-width fields and bounded arrays, not pointers, virtual members, heap-owned strings, or platform-dependent layout.

7. Store frequently changing retained state separately from configuration.

   Some device values are not configuration but still need to survive reboot. A switch's last output state is the example: it may change often and can be used on startup if the device is configured to restore the previous state. Such values should live in a separate retained-state store keyed by `DeviceId`, not in the device config payload. The retained-state store should be bounded, type-specific, optional per device type, and write-debounced or otherwise coalesced to reduce NVS wear. Config payloads remain the user's intended setup; retained state captures selected runtime values used for restoration.

8. Use `uint32_t DeviceId` for identity and allow duplicate names.

   Device IDs are generated when a device is created, never changed by rename or configuration updates, and stored as `uint32_t` values. Firmware generation can use `esp_random()` and MUST reject `0`, check for duplicates in the current registry, and retry a bounded number of times before failing the create request. Names are display labels and can duplicate. Relationship fields, events, commands, and API payloads use `DeviceId`. Integrations that need globally unique IDs derive them from controller identity plus device ID, for example `<controller-id>_<device-id-hex>`. UUID strings were considered, but they are unnecessarily large for ESP32 NVS and RAM when the expected local registry size is on the order of 100-200 devices.

9. Let device types declare relationship constraints.

   Each device type should expose metadata through a type descriptor, for example type id, supported parent type(s), whether it can have children, max children if bounded, required config fields, and command support. The registry uses these descriptors for create/update validation. This makes DS18B20-on-OneWire and future I2C device rules data-driven instead of scattering type-specific `if` chains through Web UI and integration code. The alternative of hardcoded validation in route handlers would duplicate behavior and allow transports to disagree.

10. Make deletion restrictive by default.

   Deleting a device with children is rejected unless a future explicit cascade operation is introduced. Deleting a leaf device moves the runtime instance through a stopping/deleting state, removes the per-device record and index entry only after validation, publishes events, and frees the slot. Disabled devices remain in the registry. This protects bus relationships from accidental deletion. The alternative of automatic cascade deletion is convenient but dangerous through remote integrations and hard to recover from on a small controller.

11. Separate persisted enablement from effective runtime status.

   Persisted configuration answers whether the user wants a device enabled. Runtime status answers whether it is currently usable. A child whose parent bus is disabled, faulted, missing, or deleting reports an effective dependency-blocked status even when the child remains enabled in NVS. When the parent recovers, enabled children are allowed to start again. This preserves user intent across transient hardware failures. The alternative of rewriting child enabled flags during parent changes would lose intent and cause unnecessary NVS writes.

12. Use multi-rate cooperative ticks and lifecycle state machines only where they add value.

   `App` should compute `now` once per loop pass and schedule multiple cooperative cadences, for example a fast-loop tick, a 100 ms tick, and a 1 s tick. The registry can route each cadence only to devices or services that declare they need it, so 100-200 devices do not all run fast-path logic unnecessarily. Runtime devices that have multi-step start, stop, retry, read, scan, or reconfigure flows can inherit `StateMachine` or use a small equivalent adapter. `DummyDevice` should exercise lifecycle states without artificial blocking waits. Domain state handlers MUST use the App-provided `now` for their cadence and must not call `millis()` or `clock_.millis()`. API mutations do not need to carry timestamps; timestamp-dependent debounce or dirty flush behavior should be resolved on a later registry tick.

13. Publish normalized events through a small integration event bus.

   Registry and runtime changes emit `DeviceEvent` records such as registry-loaded, device-created, device-updated, device-deleted, status-changed, state-changed, command-accepted, command-rejected, and config-persisted. Events carry a registry revision, device ID, type, event kind, and bounded payload/detail fields. Integrations implement an interface such as `IDeviceIntegration` with `begin(...)`, cadence-specific ticks, `onDeviceEvent(...)`, and command submission into the registry. The event bus fans out to registered integrations without knowing MQTT, WebSocket, or Home Assistant details. The alternative of calling MQTT/WebSocket code directly from the registry would make the registry untestable and transport-specific.

14. Route all external commands through the same registry service.

    Web UI routes and integration adapters submit normalized commands: create, update config, delete, enable, disable, rename, set status for DummyDevice, or type-specific commands. The registry validates permissions implied by type metadata, current lifecycle status, relationships, bounds, and the operation's persistence policy before accepting a mutation. Immediate operations only return success after required NVS writes succeed; delayed operations return success with pending persistence marked. Failed commands return explicit errors and emit command-rejected events. This prevents a remote integration from bypassing rules enforced in the UI.

## Risks / Trade-offs

- [Risk] 32-bit random IDs can collide. -> Mitigation: reserve `0`, check generated IDs against the loaded registry, retry a bounded number of times, and return a clear create failure if no unique ID is produced.
- [Risk] NVS space can fill if device payloads grow without bounds. -> Mitigation: enforce max device count, max index size, max record size, max type-config size, and reject writes before committing.
- [Risk] Rewriting the registry too often can wear NVS. -> Mitigation: persist only accepted configuration changes, never telemetry, readings, or transient status updates.
- [Risk] Delayed persistence can lose accepted in-memory changes on sudden power loss. -> Mitigation: reserve immediate persistence for structural registry changes, expose pending persistence for delayed mutations, and provide forced flush before controlled restarts.
- [Risk] Retained runtime state can wear NVS if written on every output change. -> Mitigation: keep retained state separate from config, make it opt-in per device type, and use bounded debounce/coalescing before writes.
- [Risk] Binary config migration code can corrupt records if old layouts are edited or copied incorrectly. -> Mitigation: keep old config structs immutable, use fixed-width fields, static size checks, and test migrations from byte fixtures.
- [Risk] Wall-clock time can be unavailable or stale at boot. -> Mitigation: use monotonic revisions for ordering and treat `updatedAt` as optional display metadata only.
- [Risk] Parent status propagation can create confusing child states. -> Mitigation: expose both persisted enabled state and effective runtime status, and include parent device ID/status detail in child status events.
- [Risk] Event delivery can block the cooperative loop if integrations perform network work synchronously. -> Mitigation: integration `onDeviceEvent` must enqueue or coalesce quickly; network publishing happens from `tick(now)`.
- [Risk] A fast tick over every device can become expensive at 100-200 devices. -> Mitigation: let device type descriptors declare tick cadences and route only due cadence ticks to interested runtime instances.
- [Risk] Type-specific validation can become a dispatcher. -> Mitigation: keep validation in focused device type descriptors/factories and split real buses/sensors into their own files when added.
- [Risk] Delete and reconfigure flows can race with runtime ticks. -> Mitigation: make registry operations synchronous at the configuration boundary and request runtime stop/restart through explicit lifecycle states.

## Migration Plan

1. Add the new registry storage namespace and treat missing registry data as an empty registry with the current schema version.
2. Add host tests for empty load, valid load, corrupt index recovery, corrupt device record recovery, retained-state recovery, save/load round-trip, and size/count validation.
3. Add type-specific config versioning and migration tests before relying on binary device payloads for real devices.
4. Add dirty queues and immediate/delayed/coalesced persistence policy handling.
5. Add retained-state storage with bounded writes for `DummyDevice` or a switch-like dummy state before adding a real GPIO switch.
6. Add `DummyDevice` and restore it from persisted records on boot.
7. Add App-level cadence scheduling and route registry/device work through the relevant cadence-specific ticks.
8. Add Web UI/API operations for list/create/update/delete/enable/disable/command using the same registry service as integrations.
9. Add integration event interfaces and a test integration sink before adding real MQTT/WebSocket/Home Assistant adapters.
10. Rollback for early development is to erase the device-registry namespace or reject the registry index/payload records and boot with an empty dynamic registry; controller `DeviceConfig` remains independent.

## Open Questions

- What is the initial maximum device count for ESP32 builds: 100, 200, or a board-profile-specific value based on measured RAM/NVS usage?
- Should external JSON represent `DeviceId` as unsigned integer, fixed-width hex string, or support both while storing `uint32_t` internally?
- Which default tick cadences should exist in the first implementation: fast-loop + 100 ms + 1 s, or only fast-loop + 1 s until a device needs 100 ms?
- Should the first retained-state implementation be generic per type or only the minimum needed for a switch-like last-state restore path?
- Should retained-state writes use a fixed debounce interval, an explicit flush-on-idle policy, or both?
- Which operations are immediate by default in the first slice: create/delete/relationship only, or also enable/disable?
- Should API responses include only a boolean `pendingPersistence` or separate pending flags for index, config record, and retained state?
- Should the first Web UI expose only `DummyDevice`, or also allow creating placeholder bus/sensor descriptors before real drivers exist?
- Should future cascade delete be supported as an explicit advanced operation, or should users always delete children first?
