## 1. Compact modal structure

- [x] 1.1 Rework `DeviceDetailDialog.vue` so view, edit, and create use one compact section grammar instead of separate visual rhythms.
- [x] 1.2 Render view-mode fields with readonly input shells and keep create-mode shells aligned so labels, heights, and alignment match across modes.
- [x] 1.3 Add visible section surface treatment and reduce body padding so the detail card no longer reads as a single flat plane.
- [x] 1.4 Keep the existing modal behavior intact while tuning spacing, section headers, and body density for the new layout.

## 2. GPIO detail density

- [x] 2.1 Refactor `GpioSwitchDeviceForm.vue` and `GpioSwitchDeviceDetail.vue` so primary GPIO fields stay prominent and the secondary config disclosure reads as a compact grouped block.
- [x] 2.2 Align detail labels so they sit closer to the field boundary and match the edit-mode field rhythm.
- [x] 2.3 Ensure quick commands remain outside the collapsed config section and continue to behave the same in view mode.

## 3. Verification

- [x] 3.1 Validate the modal at desktop and mobile breakpoints to confirm the GPIO section remains visible earlier in the viewport and no empty blocks are rendered.
- [x] 3.2 Verify view/edit/create parity so the shared modal surface uses the same compact section structure in all modes.
- [x] 3.3 Run the project checks relevant to the touched SPA files and confirm formatting and build integrity after the UI update.
