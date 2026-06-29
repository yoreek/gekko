# Device Registry Persistence

The dynamic device registry is stored in the `device_registry` NVS namespace. It is separate from controller-level configuration and retained runtime state.

## Storage Shape

- `version` stores the registry storage format version.
- `index` stores a bounded binary index record.
- `record_%08x` stores one binary device record by `DeviceId`.

The index record contains fixed metadata plus fixed-size entries capped by `kMaxDynamicDevices` and `kMaxRegistryIndexBytes`. Each entry stores the `DeviceId` and `DeviceTypeId`; the record key is derived from `DeviceId`.

The registry persistence path must not build the index as a heap `std::string`, heap `std::vector`, or unbounded serialized buffer.

## Commit Order

Registry flush writes dirty device records before writing the index. The registry format `version` is written after the index, so it is the final commit marker for a supported registry format.

Create can leave an orphan `record_*` if record write succeeds and index commit fails. This is acceptable because boot loads only records referenced by the committed index.

Delete writes the new index first. Cleanup of the deleted `record_*` happens only after the index and format marker are committed.

Config-only changes write only dirty device records. Rename is a record-only change and must not rewrite the index.

## Boot Recovery

On boot, the firmware validates the registry format version, index, referenced device records, type support, and relationships before creating runtime instances.

If the registry format version is missing or unsupported, or if the registry snapshot is corrupt, the firmware clears the dynamic registry namespace and continues boot with an empty dynamic registry. This avoids requiring a manual NVS erase after incompatible registry format changes.

## Device Config Payloads

Device configs are stored as bounded binary payloads owned by the concrete device type. Public REST create/update requests use JSON, but encoding to the internal binary payload stays inside firmware adapters.

Device runtime snapshots expose current runtime state, such as switch output or DS18B20 temperature, separately from persisted config.

Display layouts are a special case of persisted device-scoped state:

- the raw widget `text` is persisted as part of the layout payload
- compiled text AST data is runtime-only and is rebuilt from text after load
- referenced display source devices are persisted separately as `metric_source` dependency links on the display device record
