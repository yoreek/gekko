## Why

Device details currently expose technical fields, but the three most important switch configuration values are still easy to misread: `Startup state`, `Safe state`, and `Restore previous state`. The detail dialog also behaves as a read-only viewer, so users cannot adjust supported fields directly where they inspect them.

## What Changes

- Add contextual help hints to the key switch fields in device details.
- Add a dedicated edit mode to the device detail dialog so supported fields can be modified in-place.
- Keep the edit flow consistent across dashboard and Devices page entry points.
- Add an `Actions` column to the Devices table with `Edit` and `Delete` actions so destructive operations stay in the list view instead of the detail dialog.
- Keep the open detail dialog and Devices table synchronized after edit and delete actions by applying the returned snapshot and subsequent realtime device updates without a full registry reload.
- Make the hint pattern reusable so more fields can be annotated later if needed.

## Capabilities

### Modified Capabilities
- `device-dashboard-ui`: device detail modal gains contextual field hints and an edit mode for supported device types.
- `device-registry-table-ui`: the Devices page detail dialog reuses the same edit-mode and hint behavior when opened from the table, and the table gains an `Actions` column with `Edit` and `Delete`.

## Impact

- Frontend `DeviceDetailDialog` and type-specific detail components.
- Type-specific edit forms for devices that support editable config.
- No new backend API is required if existing update-config and command endpoints are reused.
- Existing test coverage should be extended for readonly view, edit mode, save, and cancel behaviors.
