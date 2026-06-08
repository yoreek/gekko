## ADDED Requirements

### Requirement: Dynamic device identity
The firmware SHALL assign every dynamic device a stable 32-bit `DeviceId` identity and SHALL treat display names as non-unique labels.

#### Scenario: Device is created
- **WHEN** a caller creates a dynamic device without a device ID
- **THEN** the firmware assigns a non-zero `DeviceId`, stores the requested type and display name, and returns the `DeviceId` as the stable device identifier

#### Scenario: Device is renamed
- **WHEN** a caller changes a device display name
- **THEN** the firmware preserves the device ID and accepts the name even when another device already uses the same name

#### Scenario: Generated ID collides
- **WHEN** generated device ID already exists in the loaded registry
- **THEN** the firmware retries ID generation a bounded number of times before failing the create request with a clear validation error

### Requirement: Registry validation
The firmware SHALL validate registry mutations before changing memory state or persisted NVS state.

#### Scenario: Unsupported type is rejected
- **WHEN** a caller tries to create a device with an unsupported type
- **THEN** the firmware rejects the request and leaves the in-memory and persisted registry unchanged

#### Scenario: Invalid device ID reference is rejected
- **WHEN** a caller references a missing, zero, or malformed device ID
- **THEN** the firmware rejects the request with a validation error and leaves the registry unchanged

#### Scenario: Registry bounds are enforced
- **WHEN** a create or update request would exceed the supported device count, name length, type config size, per-record size, or index size
- **THEN** the firmware rejects the request before writing to NVS

### Requirement: NVS-backed registry persistence
The firmware SHALL persist accepted dynamic device registry changes as a versioned index plus per-device records in a NVS-backed storage area separate from controller-level configuration.

#### Scenario: Empty registry is loaded
- **WHEN** the device boots and no dynamic device registry exists in NVS
- **THEN** the firmware creates an empty in-memory registry with the current registry schema version

#### Scenario: Existing registry is loaded
- **WHEN** the device boots and NVS contains a supported dynamic device registry
- **THEN** the firmware validates the registry version, index, per-device records, and type support before rebuilding the in-memory registry and creating runtime instances for enabled records

#### Scenario: Corrupt registry is handled
- **WHEN** the device boots and NVS contains a corrupt or unsupported dynamic device registry
- **THEN** the firmware logs the issue, does not instantiate unsafe records, and continues boot using the controller configuration

#### Scenario: Accepted mutation is applied
- **WHEN** a caller creates, updates, enables, disables, or deletes a dynamic device and validation succeeds
- **THEN** the firmware updates the in-memory registry immediately and applies the operation's persistence policy to the affected index, device record, or retained-state record

### Requirement: Operation-level persistence policy
The firmware SHALL support immediate, delayed, and coalesced persistence policies for registry, configuration, and retained-state mutations.

#### Scenario: Immediate mutation succeeds
- **WHEN** a mutation with immediate persistence policy is accepted
- **THEN** the firmware writes the required NVS index or record changes before returning success and reports no pending persistence for that mutation

#### Scenario: Immediate mutation fails to persist
- **WHEN** a mutation with immediate persistence policy cannot write the required NVS changes
- **THEN** the firmware rejects or rolls back the in-memory mutation and reports the operation as failed

#### Scenario: Delayed mutation is accepted
- **WHEN** a mutation with delayed persistence policy is accepted
- **THEN** the firmware updates the in-memory registry, marks the affected index or device record dirty, and reports pending persistence for the affected scope

#### Scenario: Coalesced retained-state mutation is accepted
- **WHEN** a retained-state value changes repeatedly before the flush policy is due
- **THEN** the firmware keeps only the latest retained-state value dirty for that device and avoids writing every intermediate value to NVS

#### Scenario: Dirty records are flushed
- **WHEN** the dirty flush debounce or max-delay policy is reached on an App-scheduled registry tick
- **THEN** the firmware writes dirty index, config records, and retained-state records to NVS and clears dirty flags only for writes that succeed

#### Scenario: Forced flush is requested
- **WHEN** the firmware is about to perform a controlled reboot, OTA restart, factory reset, or explicit save/apply action
- **THEN** the firmware attempts to flush pending dirty registry, config, and retained-state records before continuing the controlled action

### Requirement: Type-specific config version migration
The firmware SHALL version each device type's binary configuration payload independently from the registry index and SHALL migrate old supported payload versions through the owning device type descriptor.

#### Scenario: Current config version is loaded
- **WHEN** a device record contains the current config version for its device type
- **THEN** the firmware parses the payload using the current immutable config layout, validates the result, and creates the runtime device

#### Scenario: Older supported config version is loaded
- **WHEN** a device record contains an older supported config version for its device type
- **THEN** the firmware parses the payload using the matching old immutable config layout, migrates it through the device type descriptor, validates the migrated config, and marks the record for rewrite in the current config version

#### Scenario: Unsupported config version is loaded
- **WHEN** a device record contains an unsupported config version for its device type
- **THEN** the firmware does not instantiate that device record and reports a recoverable registry load issue without blocking controller boot

#### Scenario: Old config layout is preserved
- **WHEN** a released binary config layout is superseded by a newer version
- **THEN** the old layout remains available for migration tests and is not modified to match the new layout

### Requirement: Configuration and registry revisions
The firmware SHALL keep configuration layout version, per-device configuration revision, and registry revision as separate concepts.

#### Scenario: Device config mutation is accepted
- **WHEN** a caller accepts a device configuration mutation
- **THEN** the firmware increments that device's config revision and the registry revision in memory and reports pending persistence when the mutation uses delayed persistence

#### Scenario: Runtime status changes
- **WHEN** a device runtime status changes without changing configuration
- **THEN** the firmware does not increment the device config revision and does not rewrite the device config record

#### Scenario: Delayed config persistence succeeds
- **WHEN** a dirty device configuration record is later written to NVS successfully
- **THEN** the firmware clears pending persistence for that record without incrementing the device config revision again

#### Scenario: Wall-clock timestamp is unavailable
- **WHEN** wall-clock time is unavailable, stale, or not synchronized
- **THEN** the firmware still orders registry/config changes using revision counters and does not require `updatedAt` metadata

#### Scenario: Wall-clock timestamp is present
- **WHEN** the firmware stores `updatedAt` or similar time metadata
- **THEN** the timestamp is treated as optional display metadata and is not used for migration, ordering, or correctness checks

### Requirement: Retained runtime state storage
The firmware SHALL persist selected frequently changing restore values separately from device configuration records.

#### Scenario: Switch-like last state changes
- **WHEN** a switch-like device changes its output state and the device type supports restoring the previous state
- **THEN** the firmware records the retained state in a separate retained-state storage area instead of modifying the device configuration payload

#### Scenario: Device restores retained state
- **WHEN** a device starts and its configuration requests restore-from-previous behavior
- **THEN** the firmware loads the device's retained state by device ID and applies it as startup state when the retained state is valid for that device type

#### Scenario: Retained state is missing
- **WHEN** a device requests restore-from-previous behavior but no valid retained state exists
- **THEN** the firmware uses the device type's configured fallback startup state

#### Scenario: Retained state writes are bounded
- **WHEN** retained state changes repeatedly in a short interval
- **THEN** the firmware coalesces, debounces, or otherwise bounds retained-state writes to avoid excessive NVS wear

### Requirement: Device lifecycle status
The firmware SHALL expose lifecycle status for each dynamic device from creation through configuration, runtime operation, disabling, fault handling, reconfiguration, and deletion.

#### Scenario: Device starts after creation
- **WHEN** a valid enabled device is created
- **THEN** the firmware moves the device through an explicit creation/start lifecycle and exposes its resulting runtime status

#### Scenario: Device configuration changes
- **WHEN** a caller updates a valid configuration for an existing enabled device
- **THEN** the firmware records the accepted configuration revision, applies the operation's persistence policy, and moves the runtime instance through an explicit reconfiguration or restart lifecycle before reporting it ready

#### Scenario: Device is disabled
- **WHEN** a caller disables a dynamic device
- **THEN** the firmware applies the operation's persistence policy to the disabled state, stops runtime activity for that device, and reports the device as disabled without deleting its configuration

#### Scenario: Device is deleted
- **WHEN** a caller deletes a dynamic device that is allowed to be deleted
- **THEN** the firmware stops the runtime instance, removes the persisted record, and no longer lists the device as active

### Requirement: DummyDevice first implementation
The firmware SHALL include a `DummyDevice` type that exercises registry persistence, lifecycle status, commands, and integration events without requiring hardware.

#### Scenario: DummyDevice survives reboot
- **WHEN** a `DummyDevice` is created and the firmware restarts
- **THEN** the firmware restores the `DummyDevice` from NVS with the same device ID, name, enabled state, and configuration revision

#### Scenario: DummyDevice status command
- **WHEN** a caller sends a supported `DummyDevice` command that changes its simulated state
- **THEN** the firmware validates the command, updates the runtime status cooperatively, and emits the corresponding device event

### Requirement: Cooperative device runtime
The firmware SHALL keep dynamic device runtime work cooperative, timing-aware, and scheduled by cadence.

#### Scenario: App schedules runtime cadences
- **WHEN** the application loop computes the current timestamp
- **THEN** the firmware uses that App-provided `now` value for fast-loop, 100 ms, 1 s, or equivalent due runtime cadences

#### Scenario: Runtime device is ticked at declared cadence
- **WHEN** a dynamic device declares that it needs a runtime cadence
- **THEN** the firmware invokes the device only for due cadences it declares and passes the App-provided `now` timestamp into the handler

#### Scenario: Runtime work requires waiting
- **WHEN** a dynamic device needs retries, delayed transitions, hardware waits, or multi-step reconfiguration
- **THEN** the firmware represents the flow with `StateMachine` or an equivalent explicit state-machine adapter

#### Scenario: Domain handler uses provided time
- **WHEN** a device or registry state handler evaluates deadlines, debounce, dirty flush, retries, or delayed transitions
- **THEN** it uses the App-provided `now` value for that cadence and does not call `millis()` or `clock_.millis()` inside the domain handler
