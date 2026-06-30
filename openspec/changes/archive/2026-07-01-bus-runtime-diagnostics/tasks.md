## 1. Shared Diagnostics Core

- [ ] 1.1 Add a shared bus diagnostics helper/state that holds `consecutiveErrors`, `lastErrorCode`, `lastErrorAtMs`, `errorOps`, reset behavior, and debounce scheduling without persisting anything in config.
- [ ] 1.2 Add a named `resetDiagnostics` command path for supported bus runtimes so the API can clear diagnostics without reconfiguring the device.
- [ ] 1.3 Extend bus runtime snapshot serialization to emit nested `runtime.diagnostics` for I2C and SPI devices only.

## 2. I2C Scan Runtime

- [ ] 2.1 Implement cooperative on-demand I2C scan state in the bus runtime so one address is probed per tick and scan progress remains non-blocking.
- [ ] 2.2 Record I2C scan results in runtime state and publish the completed scan snapshot when the scan finishes.
- [ ] 2.3 Update I2C error bookkeeping so failed scan or bus operations increment the runtime diagnostics counters and preserve the last error metadata.

## 3. SPI Runtime Diagnostics

- [ ] 3.1 Wire the shared diagnostics helper into the SPI bus runtime so SPI reports the same runtime diagnostics fields as I2C.
- [ ] 3.2 Keep SPI discovery out of scope by ensuring SPI does not expose scan behavior or address discovery commands.
- [ ] 3.3 Apply debounce rules so diagnostic-only SPI updates are coalesced while immediate updates still occur for reset or visible state changes.

## 4. Portal API And Realtime

- [ ] 4.1 Update bus device API adapters and controllers to serialize nested diagnostics and support `resetDiagnostics` plus the I2C `scan` command.
- [ ] 4.2 Update realtime `device.upsert` and `device.command_result` payload generation so bus diagnostics and I2C scan state merge directly into the frontend store.
- [ ] 4.3 Update mock data and any typed portal contracts so the SPA and tests understand the nested diagnostics snapshot shape.

## 5. SPA Display And Tests

- [ ] 5.1 Update bus detail views to show a dedicated diagnostics section, I2C scan state, and a reset diagnostics action.
- [ ] 5.2 Add or update locale strings and view models so diagnostics labels remain consistent across the UI.
- [ ] 5.3 Add targeted tests for diagnostics reset, debounce behavior, I2C scan progression, API serialization, realtime merge behavior, and the bus detail UI.
