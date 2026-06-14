## 1. Contract and Specs

- [x] 1.1 Finalize the realtime snapshot contract in the change specs
- [x] 1.2 Confirm the portal realtime and web app specs reflect direct store merge behavior

## 2. Backend Realtime Snapshot Emission

- [x] 2.1 Build canonical device snapshot payloads for websocket upsert and command result events
- [x] 2.2 Reuse the existing device type adapter serialization path so websocket output matches REST snapshots
- [x] 2.3 Keep device remove messages lightweight and identity-only
- [x] 2.4 Add or update backend tests for snapshot-based websocket payloads

## 3. Frontend Store Merge

- [x] 3.1 Update the realtime bridge to accept canonical device snapshots
- [x] 3.2 Keep legacy nested payload tolerance only as a transition fallback
- [x] 3.3 Ensure the device registry store updates revision and pending-persistence flags without full reload
- [x] 3.4 Align mock realtime snapshot and command emission with the canonical payload shape

## 4. Verification

- [x] 4.1 Verify that one device command produces one command request and one websocket-driven store update
- [x] 4.2 Verify dashboard and device detail views update from store changes without a full registry refetch
- [x] 4.3 Run the firmware test suite and a browser smoke check after the change
