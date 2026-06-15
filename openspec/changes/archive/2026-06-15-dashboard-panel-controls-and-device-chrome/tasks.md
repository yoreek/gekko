## 1. Dashboard panel controls

- [ ] 1.1 Keep the dashboard active-panel controls as explicit left/right move buttons in the header area.
- [ ] 1.2 Wire the move buttons to the existing panel reorder flow so the saved layout state remains unchanged.
- [ ] 1.3 Confirm the controls remain disabled when the active panel cannot move further in that direction.

## 2. Device detail chrome

- [ ] 2.1 Add a compact `View` / `Edit` indicator to the `DeviceDetailDialog` header.
- [ ] 2.2 Keep the indicator localized and aligned with the existing title/status row.
- [ ] 2.3 Remove the redundant `Type-specific details` wrapper title from the modal composition.

## 3. Verification

- [ ] 3.1 Verify the dashboard still reorders panels correctly after using the explicit move controls.
- [ ] 3.2 Verify the device detail modal still renders shared fields, type-specific fields, and the current mode indicator without extra chrome.
- [ ] 3.3 Run the relevant project checks for the touched SPA files and confirm the change is consistent with the existing saved layout behavior.
