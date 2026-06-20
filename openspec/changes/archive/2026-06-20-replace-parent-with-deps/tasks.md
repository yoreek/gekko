## 1. Core Dependency Data Model

- [ ] 1.1 Add bounded dependency role/link types, role string parsing/formatting, and `kMaxDeviceDependencies`.
- [ ] 1.2 Replace `hasParent` and `parentDeviceId` in registry records and create/update requests with `deps` storage.
- [ ] 1.3 Make `has_deps` a computed API/model field derived from `deps` length and ensure it is not persisted.
- [ ] 1.4 Replace `compatibleParentTypes`, `canHaveChildren`, and `maxChildren` descriptor metadata with dependency/dependent metadata.
- [ ] 1.5 Update binary codec and tests for the dependency-aware record format, including the chosen old-record migration or reset behavior.

## 2. Registry And Runtime Wiring

- [ ] 2.1 Rename registry helpers from child/parent to dependent/dependency, including delete rejection result fields.
- [ ] 2.2 Update snapshot validation for missing deps, self deps, duplicate roles, dependency type compatibility, dependent limits, and cycles through deps.
- [ ] 2.3 Replace `setParent` and `SetParent` command handling with dependency-aware mutation APIs and `set_deps`.
- [ ] 2.4 Rename runtime base APIs from `parentRuntime`/`childRuntimes` to role-addressable dependency and dependent runtime APIs.
- [ ] 2.5 Update effective status propagation and reconfiguration cascade to scan all dependency links and derived dependents.
- [ ] 2.6 Add native registry/runtime tests for deps persistence, derived dependents, delete protection, reassignment rollback, status propagation, and old naming removal.

## 3. DS18B20 Migration

- [ ] 3.1 Convert DS18B20 descriptor validation from compatible parent type to required `onewire_bus` dependency role.
- [ ] 3.2 Update DS18B20 runtime parent bus access to resolve the OneWire dependency by role.
- [ ] 3.3 Update duplicate OneWire address detection to use derived dependents/runtime dependents rather than children.
- [ ] 3.4 Update DS18B20 REST adapter create/update parsing to use `deps` and remove parent fields.
- [ ] 3.5 Update DS18B20 tests for create, update, invalid dependency, duplicate address, runtime wiring, and snapshots.

## 4. REST WebSocket And SPA

- [ ] 4.1 Update common REST serialization and streamed device list output to emit `deps` and computed `has_deps`, never `has_parent` or `parent_device_id`.
- [ ] 4.2 Update controller command parsing to accept `set_deps` and reject migrated parent-shaped commands.
- [ ] 4.3 Update websocket device snapshots to emit the same deps-shaped contract as REST.
- [x] 4.4 Update SPA API contracts, device models, stores, realtime merge, mock database, and mock handlers for `deps` and `hasDeps`.
- [ ] 4.5 Update DS18B20 form/detail/components/i18n to select and display OneWire dependency wording rather than parent wording.
- [x] 4.6 Add focused SPA verification for DS18B20 create/edit payloads, mock mode, realtime merge, and absence of parent fields.

## 5. Verification

- [ ] 5.1 Run `scripts/test.sh` and fix firmware/native regressions.
- [x] 5.2 Run the project SPA verification command when frontend files are changed.
- [ ] 5.3 Search the codebase for removed public/internal names and eliminate stale `has_parent`, `parent_device_id`, `set_parent`, `parentRuntime`, and `childRuntimes` usage.
- [ ] 5.4 Review memory/cooperative constraints: bounded deps arrays, no hot-path heap churn beyond existing runtime vectors, no blocking waits, and no direct `Serial` logging.
