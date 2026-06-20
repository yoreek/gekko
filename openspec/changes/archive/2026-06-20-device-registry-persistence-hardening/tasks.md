## 1. Index Storage Model

- [x] 1.1 Replace full-index serialization with sequential index entry persistence using only fixed cursor/header state plus one index entry or a small fixed page in RAM.
- [x] 1.2 Store only the device id and the information needed to load the matching device record in each index entry; derive the record key from the id when possible.
- [x] 1.3 Use bounded staged index pages plus a small commit metadata record if the storage layer cannot append/update entries without materializing the full index value.
- [x] 1.4 Ensure registry load reads only the committed index metadata/generation and ignores incomplete staged index data.

## 2. Persistence Commit Order

- [x] 2.1 Update registry persistence so dirty per-device records are written before staged index entries/pages and the index commit metadata.
- [x] 2.2 Keep failed save attempts from exposing a broken committed registry on the next boot.
- [x] 2.3 Update `DeviceRegistry::flushNow()` to write only dirty config records and to write index entries/pages only when `dirtyIndex()` is true.
- [x] 2.4 Treat delayed rename as a record-only dirty write without index rewrite.
- [x] 2.5 Keep create/delete structural persistence ordered as record writes, staged index entries/pages, final index metadata commit, and stale record cleanup only after successful index commit.
- [x] 2.6 Clear the selected dirty batch as one unit after the flush attempt and do not add per-record dirty recovery bookkeeping.

## 3. Regression Coverage

- [x] 3.1 Add an index persistence test proving memory use does not require a full serialized index buffer.
- [x] 3.2 Add a save-failure regression test that proves the committed index metadata is not advanced until record writes and staged index writes succeed.
- [x] 3.3 Add a load regression test for a partially written registry snapshot and confirm the previous committed registry remains recoverable.
- [x] 3.4 Add a selective-flush regression test proving an update to one device does not rewrite unrelated device records.
- [x] 3.5 Add a delayed-rename regression test proving rename writes the renamed record without rewriting the index.
- [x] 3.6 Add create/delete ordering tests for orphan record tolerance and deleted-record cleanup after index commit.
- [x] 3.7 Run the relevant registry and persistence tests after the change.

## 4. Device List Iteration

- [x] 4.1 Replace `DeviceRegistry::list()` copy behavior with a read-only iteration helper over the snapshot records.
- [x] 4.2 Update `DeviceRegistryController::index()` to stream JSON directly from the snapshot iterator helper.
- [x] 4.3 Add or update controller tests to cover the direct-iteration path without requiring a copied vector.
