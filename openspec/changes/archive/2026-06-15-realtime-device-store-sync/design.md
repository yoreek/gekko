## Context

The portal already loads `/api/devices` once on bootstrap and keeps the device registry in Pinia. However, realtime device events currently arrive in different shapes depending on source:

- real backend websocket events are sparse and do not contain a full snapshot;
- mock realtime uses a different wrapper shape;
- the frontend bridge only merges one of the mock-compatible payload variants.

This breaks the agreed workflow where websocket updates should mutate the local store directly and avoid a full registry refetch.

## Goals / Non-Goals

**Goals:**
- Make websocket device update payloads canonical and snapshot-based.
- Ensure frontend store updates happen from realtime messages without a registry reload.
- Keep `device.remove` lightweight and identity-only.
- Keep mock mode aligned with real mode.

**Non-Goals:**
- Do not redesign the dashboard layout or device widgets.
- Do not add a websocket channel for dashboard layout.
- Do not change the bootstrap REST fetch contract.

## Decisions

1. **Use a flat full device snapshot for `device.upsert` and `device.command_result`.**
   - Rationale: the frontend store already normalizes `DeviceRecord` snapshots, so a flat payload can merge directly without reconstructing missing state.
   - Alternative considered: keep sparse events and patch store fields manually. Rejected because sparse payloads are easy to drift and cannot safely represent renamed/config-changed devices.

2. **Build websocket snapshots from the current registry record plus runtime adapter data.**
   - Rationale: the backend already serializes full device snapshots for REST. Reusing the same adapter path keeps REST and websocket output consistent.
   - Alternative considered: introduce a second websocket-only DTO. Rejected because it would duplicate serialization rules.

3. **Make the frontend bridge accept the canonical snapshot payload and tolerate the legacy nested form during transition.**
   - Rationale: the bridge should remain robust while backend and mock code are updated together.
   - Alternative considered: hard cut to one payload shape with no compatibility. Rejected because it makes local verification brittle.

4. **Align mock realtime with the same canonical shape.**
   - Rationale: mock mode should exercise the same merge path as real mode so payload drift is visible during development.

## Risks / Trade-offs

- [Extra serialization work on each device event] -> Mitigate by reusing existing bounded adapter serialization and only snapshotting the affected device.
- [Temporary compatibility complexity in the bridge] -> Mitigate by keeping the legacy fallback small and removing it once the real and mock paths are verified.
- [Potential mismatch between event timing and store revision] -> Mitigate by keeping revision metadata in the websocket envelope and updating the store revision before the snapshot merge.

## Migration Plan

1. Update backend websocket message generation to emit canonical device snapshots.
2. Update the frontend bridge to merge the canonical payload.
3. Align mock realtime snapshot and command emission to the same contract.
4. Verify with browser smoke tests that one command results in one command request and a websocket-driven store update.

Rollback is straightforward: restore the previous websocket payload shape and the legacy bridge parsing if the canonical snapshot path exposes an unexpected runtime issue.
