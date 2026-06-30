## Why

The firmware already has bus-owned runtimes for I2C and SPI, but their live state is too thin for troubleshooting: the UI can see only basic readiness, while transient bus faults, repeated failures, and scan results are not surfaced in a compact, consistent way. The new runtime diagnostics layer adds visible bus health without moving anything into persisted config or spamming the portal with every low-level error.

## What Changes

- Add a shared runtime diagnostics model for bus-backed devices with a nested `runtime.diagnostics` snapshot.
- Keep diagnostics runtime-only: do not persist them in config blobs, migrations, or setup bundles.
- Add a small shared mechanism for runtime publish scheduling so diagnostic updates can be debounced instead of emitted on every error.
- Add an explicit diagnostics reset action so operators can clear counters without rebooting or reconfiguring the device.
- Add on-demand cooperative I2C scan support that steps one address per tick and reports discovery results in runtime state.
- Reuse the same diagnostics envelope for SPI bus runtimes, but do not add SPI address discovery or scan behavior.
- Update portal API, realtime payloads, and device detail UI to surface the new runtime diagnostics and scan results.
- **BREAKING**: runtime snapshots gain a structured `diagnostics` object for affected bus devices; clients that read bus runtime fields must handle the nested shape.

## Capabilities

### New Capabilities
- `bus-runtime-diagnostics`: shared runtime diagnostics for I2C and SPI bus devices, including debounced runtime publication, reset support, and on-demand I2C scan reporting.

### Modified Capabilities
- `device-runtime-hierarchy`: add the shared runtime diagnostics/publish mechanism used by bus runtimes so the same error bookkeeping is not duplicated in each bus implementation.
- `portal-api-controllers`: expose runtime diagnostics in bus device snapshots and accept a structured diagnostics reset command for supported bus devices.
- `portal-realtime-state`: publish runtime diagnostics and I2C scan state in canonical device snapshots while keeping diagnostic-only updates coalesced.
- `device-dashboard-ui`: render bus diagnostics and I2C scan results in device detail views, and provide a reset action for the diagnostics state.

## Impact

- Firmware bus runtimes: `src/devices/bus/i2c/`, `src/devices/bus/spi/`, and a shared diagnostics helper in the runtime layer.
- Portal API adapters/controllers: bus device JSON serialization and command dispatch.
- Portal realtime WebSocket payloads: canonical device snapshots will include nested bus diagnostics.
- Portal SPA: bus device detail views will show diagnostics state and scan results.
- Tests: runtime bookkeeping, scan flow, command handling, API serialization, and realtime snapshot merge behavior.
