## 1. Backend Display Layout Contract

- [x] 1.1 Extend the shared display layout record and binary codec with typed widget fields and bitmap payload metadata.
- [x] 1.2 Update display layout JSON parse/write logic so `ssd1306` and `st7735` round trip `type`, `bitmapData`, `bitmapFormat`, and `keepAspectRatio`.
- [x] 1.3 Keep legacy `ssd1306` layouts readable while rejecting unsupported widget types or invalid bitmap payloads.

## 2. Display Device API and Storage

- [x] 2.1 Update `ssd1306` and `st7735` device API adapters to use the expanded display layout contract for create/update/persisted state flow.
- [x] 2.2 Adjust the display layout persisted-state/request sizing so bitmap layouts can be serialized without truncation.
- [x] 2.3 Update backend tests for display layout round trips, legacy compatibility, and validation failures.

## 3. Frontend Display Models and Forms

- [x] 3.1 Add `spiBusDeviceId` and `chipSelectPin` to the `st7735` frontend model and encode/decode logic.
- [x] 3.2 Add `st7735` form/detail components and register them in the device UI registry.
- [x] 3.3 Keep `st7735` bitmap handling color-aware with `rgb565` while preserving `ssd1306` monochrome behavior.

## 4. Designer and Bitmap Parity

- [x] 4.1 Verify the shared layout designer and bitmap import helpers preserve typed widget fields for both display types.
- [x] 4.2 Ensure the frontend device catalog and localized labels stay aligned with the updated display device contracts.
- [x] 4.3 Add or update frontend tests for `st7735` SPI config and display bitmap round trips.

## 5. Verification

- [x] 5.1 Run focused backend and frontend tests for display layout and `st7735` device flows.
- [x] 5.2 Run the project verification command set (`scripts/test.sh` and targeted SPA tests where applicable).
- [x] 5.3 Review the diff for contract drift, especially request sizes and display payload limits.
