## Why

The frontend realtime bridge currently ignores real device update events because the websocket payload shape differs between backend, mock, and store expectations. That forces state refreshes or leaves the UI stale after commands and device changes. We need one canonical device snapshot contract so realtime updates can merge directly into Pinia without a full registry reload.

## What Changes

- Standardize realtime `device.upsert` and `device.command_result` messages on a full device snapshot payload.
- Align backend websocket emission with the same device snapshot shape already used by REST device responses.
- Update the frontend realtime bridge to merge canonical device snapshots into the device store directly.
- Align mock realtime snapshot and command paths with the same payload contract.
- Keep `device.remove` as an identity-only message.

## Capabilities

### Modified Capabilities
- `portal-realtime-state`: device update messages carry a canonical device snapshot payload that the frontend can merge directly.
- `portal-web-app`: the SPA merges websocket device updates into the Pinia device store without full registry reloads and accepts the canonical snapshot payload.

## Impact

- Backend websocket message builders and portal websocket manager.
- Frontend realtime bridge, device registry store, and mock realtime helpers.
- Realtime behavior verification for dashboard and device detail flows.
