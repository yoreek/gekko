## 1. Device type contract

- [x] 1.1 Define the frontend device type catalog around numeric `type_id = 1` for `DummyDevice`
- [x] 1.2 Update device record normalization to treat numeric `type_id` as the canonical identifier
- [x] 1.3 Remove any string-based or generic device type handling from the frontend model

## 2. Dashboard UI

- [x] 2.1 Render the create-device form from the numeric type catalog
- [x] 2.2 Keep the device detail modal on the typed `DummyDevice` panel only
- [x] 2.3 Remove unsupported fallback rendering from the device detail view

## 3. Mock data and verification

- [x] 3.1 Reset the mock storage schema so old state does not leak into the updated model
- [x] 3.2 Update mock device seeds and mock create handling to use numeric `type_id`
- [x] 3.3 Update unit and smoke tests to cover the numeric type catalog and typed dashboard flow
