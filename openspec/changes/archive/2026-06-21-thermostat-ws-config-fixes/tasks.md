## 1. Firmware config and runtime visibility

- [x] 1.1 Accept thermostat config updates through both canonical fixed-point fields and Celsius aliases.
- [x] 1.2 Preserve canonical thermostat config values after update instead of falling back to defaults.
- [x] 1.3 Mark switch runtimes dirty only when the logical output actually changes.
- [x] 1.4 Ensure thermostat-driven switch output changes propagate through realtime device snapshots.

## 2. SPA thermostat detail and edit flow

- [x] 2.1 Remove the thermostat last-check row from the compact detail view.
- [x] 2.2 Keep thermostat detail rendering focused on mode, target, current temperature, desired output, and actual output.
- [x] 2.3 Fix thermostat edit Save enablement so it tracks real draft changes in config and deps.
- [x] 2.4 Update thermostat UI tests to cover config editing, snapshot-driven state, and the compact detail layout.

## 3. Verification and packaging

- [x] 3.1 Run firmware and SPA regression checks for thermostat config updates and websocket output changes.
- [x] 3.2 Rebuild SPA assets and deploy the updated firmware/web assets for live validation.
- [x] 3.3 Record the completed work in the OpenSpec change for future archive/sync steps.
