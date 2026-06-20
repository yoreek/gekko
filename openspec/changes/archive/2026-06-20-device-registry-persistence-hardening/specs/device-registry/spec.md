## MODIFIED Requirements

### Requirement: NVS-backed registry persistence
The firmware SHALL persist accepted dynamic device registry changes as a versioned index plus per-device records in a NVS-backed storage area separate from controller-level configuration.

#### Scenario: Existing registry is loaded
- **WHEN** the device boots and NVS contains a supported dynamic device registry
- **THEN** the firmware validates the registry version, index, per-device records, and type support before rebuilding the in-memory registry, creating runtime instances for the loaded records, and wiring child runtime pointers to their parent runtime objects

#### Scenario: Unsupported registry format is reset
- **WHEN** the device boots and NVS contains a missing or unsupported dynamic registry format version
- **THEN** the firmware clears the dynamic registry namespace, rebuilds an empty in-memory dynamic registry, and continues boot without requiring a manual NVS erase

#### Scenario: Accepted mutation is applied
- **WHEN** a caller creates, updates, enables, disables, or deletes a dynamic device and validation succeeds
- **THEN** the firmware updates the in-memory registry immediately, keeps live runtime objects in memory when appropriate, rewires runtime parent/child pointers for relationship changes, and applies the operation's persistence policy to the affected index, device record, or retained-state record

#### Scenario: Persisted records are written before the index
- **WHEN** the firmware flushes dirty registry data to NVS
- **THEN** it writes dirty per-device records before writing the bounded binary index record and advances the registry format commit marker only after the required records and index are durable

#### Scenario: Index is persisted as a bounded binary record
- **WHEN** the firmware writes or reads the registry index
- **THEN** it stores only fixed metadata plus fixed-size index entries capped by `kMaxDynamicDevices` and `kMaxRegistryIndexBytes`
- **AND** it does not materialize the index in a heap string, heap vector, or unbounded serialized buffer

#### Scenario: Config-only flush writes only dirty records
- **WHEN** a delayed flush is due and only device configuration records are dirty
- **THEN** the firmware writes only those dirty device records and does not rewrite unrelated device records or the registry index

#### Scenario: Rename flush writes only the renamed record
- **WHEN** a delayed rename is flushed
- **THEN** the firmware writes only the renamed device record and does not rewrite the registry index

#### Scenario: Create commits record before index
- **WHEN** a new device is persisted
- **THEN** the firmware writes the new device record before writing the index record that references the new device and before advancing the registry format commit marker

#### Scenario: Delete commits index before cleanup
- **WHEN** a device deletion is persisted
- **THEN** the firmware advances committed index metadata to an index that no longer references the deleted device before any best-effort cleanup of the deleted device record

#### Scenario: Partial save does not create a broken committed registry
- **WHEN** a persistence write fails after one or more dirty records are written but before the index write and registry format commit marker complete
- **THEN** the firmware leaves the previously committed registry as the recovery source on the next boot and does not expose an index that points at missing records

### Requirement: Operation-level persistence policy
The firmware SHALL support immediate, delayed, and coalesced persistence policies for registry, configuration, and retained-state mutations.

#### Scenario: Dirty records are flushed
- **WHEN** the dirty flush debounce or max-delay policy is reached on an App-scheduled registry tick
- **THEN** the firmware writes only dirty config records and retained-state records to NVS, writes the bounded index record and registry format commit marker last when the index is dirty, and clears the selected dirty batch as one unit after the flush attempt

#### Scenario: Flush failure is reported without dirty recovery bookkeeping
- **WHEN** a dirty registry flush cannot write one of the required records, retained states, index record, or registry format commit marker
- **THEN** the firmware returns the persistence error and does not add per-record dirty recovery bookkeeping
