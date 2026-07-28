# Blob Store

A generic key → byte-blob object store on the shared `devdata` LittleFS partition, exposed over REST at `/api/blobs/...`. It exists so any feature that needs to persist an arbitrarily-sized binary payload (e.g. an image) can do so without embedding those bytes inline in a JSON/config payload. Nothing in the firmware uses it yet — see "Not yet integrated" below.

## Storage Shape

`devdata` (`my_partitions.csv`) is mounted exactly once, in `App::begin()` (`src/core/App.cpp`), and injected by reference into every consumer that needs it. Current consumers, each confined to its own top-level directory on the shared mount:

| Consumer | Root dir |
|---|---|
| Dose journal (`LittleFsDoseJournalStorage`) | `/dj` |
| Schedule presets (`LittleFsSchedulePresetStorage`) | `/sap` |
| Blob store (`LittleFsBlobStore`) | `/blobs` (`kBlobStoreRootDir`) |

A key maps 1:1 onto a filesystem path under `/blobs` (e.g. key `foo/42/bar` → `/blobs/foo/42/bar`). This confinement is what lets a generic REST client pass an arbitrary key without ever being able to reach another consumer's data on the same mount.

## Key Format & Validation

Validated by `src/platform/BlobKeyValidation.h` before any filesystem call:

- non-empty, `<= kBlobStoreMaxKeyBytes` (96) bytes
- `/`-separated segments, `<= kBlobStoreMaxKeySegments` (8)
- each segment: charset `[A-Za-z0-9_.-]`, not exactly `.` or `..`
- no leading/trailing/doubled `/`

`isValidBlobKey("")` is deliberately `false` — this is what makes `wipeAll()` a distinct method rather than an accidental `removeByPrefix("")`.

## Write Path

`LittleFsBlobStoreCore::beginPut(key)` returns a move-only `WriteHandle`: writes go to a temp file (`<path>.tmp`), and only `commit()` renames it onto the final path (atomic replace of any existing blob at that key). An aborted or never-committed handle (including one simply destroyed) removes its own temp file and leaves any existing blob untouched. A free-space guard (`kBlobStoreMinFreeBytes` = 8192) refuses to open a handle if it would run the shared partition below the floor.

`LittleFsBlobStoreCore` is templated on the filesystem/file types (`Fs`, `FileT`) so the exact same algorithm runs against real hardware (`fs::LittleFSFS`/`File`) and against an in-memory fake in tests (`test/fakes/littlefs/FakeLittleFs.h`), rather than maintaining a parallel test implementation.

## Bulk Delete

Because keys are hierarchical paths, no separate tagging system exists or is needed for grouped deletes:

- `remove(key)` — one blob
- `removeByPrefix(prefix)` — recursively deletes everything under `prefix`, then `prefix` itself (no-op success if `prefix` doesn't exist); if `prefix` names a leaf file rather than a directory, that file alone is removed
- `wipeAll()` — clears everything under `/blobs` and recreates the root, without touching sibling directories (`/dj`, `/sap`) on the same mount

A caller that structures its keys as `<groupId>/<...>` (e.g. `<deviceId>/<random8>`) gets S3-style prefix-based bulk delete for free: `removeByPrefix(deviceId)` removes every blob belonging to that device.

## Server-Generated Keys

`beginPutGenerated(prefix, outKey)` is the untrusted-upload counterpart to `beginPut(key)`: the caller supplies only a `prefix` (a grouping label it already knows, e.g. its own device id — not secret, not collision-sensitive), and the store generates the unique part itself:

- suffix: `kBlobKeyRandomSuffixLength` (8) characters from `[A-Za-z0-9]`, via `randomAlnumChar()` (`src/platform/RandomBlobKey.h/.cpp` — `esp_random()` on device, `rand()` off-device, same `#if defined(ARDUINO)` split as `EspRandomDeviceIdSource`)
- candidate key = `prefix + "/" + suffix`, checked against the filesystem; on collision, retried up to `kBlobKeyGenerationMaxAttempts` (5) times
- `outKey` is set iff generation found a free key, independent of whether the resulting `beginPut()` call itself succeeded — this lets a caller distinguish "could not find a free key" from "found one but couldn't open it for writing"

`generateUniqueBlobKey()` is templated on `RandomCharFn`/`ExistsFn` (not on any concrete filesystem), the same dependency-injection shape as `assignUniqueDeviceId<Predicate>` (`src/devices/core/DeviceIdGenerator.h`) — this is what lets `test/test_core/test_random_blob_key.cpp` exercise the retry-on-collision path with a deterministic character sequence instead of real randomness.

This exists specifically so an untrusted caller (e.g. the SPA) can never choose or guess the unique part of a key and collide with/overwrite someone else's blob — it only ever gets to name the grouping prefix, and must persist whatever full key comes back in the response.

## REST API

Controller: `src/portal/controllers/BlobController.h/.cpp`. One controller for every feature that needs key/blob storage — not one controller per file type.

| Method | Path | Body | Behavior |
|---|---|---|---|
| `GET` | `/api/blobs/<key>` | — | Streams the blob's raw bytes (`Content-Type: application/octet-stream`), 404 if missing |
| `PUT` | `/api/blobs/<key>` | raw bytes | Uploads/replaces the blob at a caller-chosen `key` |
| `POST` | `/api/blobs/<prefix>` | raw bytes | Uploads under `prefix` with a server-generated unique key; responds `{"success":true,"key":"<prefix>/<generated>"}` |
| `DELETE` | `/api/blobs/<key>` | — | Removes the blob, or the whole subtree if `<key>` names a directory-shaped prefix (`removeByPrefix` unifies both) |
| `OPTIONS` | `/api/blobs/<...>` | — | CORS preflight |

`<key>`/`<prefix>` is everything after `/api/blobs/`, including embedded `/`, and is rejected before touching the filesystem unless it passes `isValidBlobKey`.

**Body is always raw binary**, never JSON or multipart — no `multipart/form-data` (that's reserved for OTA/MQTT-cert uploads elsewhere), no base64 (would cost ~33% size and require buffering the whole body to decode, both bad on ESP32's limited RAM). `PUT`/`POST` bodies are read via ESPAsyncWebServer's `onBody` chunked callback (genuinely TCP-fragment-sized, not whole-body-at-once), with per-request state (the in-progress `WriteHandle`, and for `POST`, the generated key) held in `request->_tempObject` across chunks and cleaned up via `request->onDisconnect(...)` if the client disconnects mid-upload. `kBlobControllerMaxPutBytes` (65536) is a sanity ceiling on top of the store's own free-space guard.

Internally, both `PUT` and `POST` dispatch with `BaseController::Action::Update`, not `Action::Create` — this is unrelated to the actual HTTP method (which is genuinely `PUT`/`POST` for CORS/routing purposes). `Action` only selects which `BaseController` hooks/virtual method fire, and the base `beforeChain()` auto-parses the body as JSON only for `Action::Create`/`Action::Cmd` — since these bodies are raw bytes, not JSON, both routes deliberately avoid that action.

## Testing

- `test/test_core/test_blob_key_validation.cpp` — key/segment validation rules
- `test/test_core/test_littlefs_blob_store.cpp` — store behavior against `test::FakeLittleFs` (round-trip, atomic replace, uncommitted-write safety, prefix/wipe deletes, root confinement, free-space guard, nested-key auto-mkdir, `beginPutGenerated`)
- `test/test_core/test_random_blob_key.cpp` — `generateUniqueBlobKey` retry-on-collision logic with a scripted deterministic character source
- `test/test_portal/test_blob_controller_path.cpp` — URL key/prefix parsing

## Consumers

**Display-widget bitmaps** (`DisplayLayoutWidgetV1::imageKey`, `src/devices/display/`) is the first (and, as of this writing, only) feature wired to this store:

- key format: `dev/<deviceId as hex>/<8-char random suffix>` — see `kDisplayLayoutImageKeyCapacity` (`DisplayLayoutStore.h`)
- upload: SPA calls `POST /api/blobs/dev/<deviceId hex>` before saving a layout; the returned `imageKey` is what's actually persisted (`DisplayLayoutBinaryWidgetV7`) — never the pixel bytes
- validation: `DisplayDeviceApiAdapter::validateLayoutImageKeys` rejects a layout referencing a missing or oversized blob at create/update time
- render: `DisplayBitmapCache.h`/`ensureDisplayBitmapBytes` reads+caches the bytes per widget (see `docs/oled-display-layout.md`)
- cleanup: `BlobStoreDeviceCleanupSink` (`src/platform/BlobStoreDeviceCleanupSink.h`) removes everything under `dev/<deviceId hex>` when the device is deleted; the SPA also deletes an old `imageKey` right after a save that replaces it with a new one (`useDisplayDesigner.save`)
- backup/restore: the NDJSON setup bundle keeps bitmap bytes portable via a dedicated `layout_bitmap` line (base64), separate from `layout_widget`'s `imageKey` — import writes the bytes back into the blob store under a freshly generated key (see `docs/backup-and-restore.md`)

Any other feature can reuse the store the same way: pick its own top-level key segment (parallel to `dev/`) so its keys never collide with another consumer's.
