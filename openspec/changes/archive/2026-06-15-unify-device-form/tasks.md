## 1. Shared Device Form Foundation

- [x] 1.1 Introduce a shared Device form shell for `view`, `edit`, and `create` modes with common fields rendered first
- [x] 1.2 Keep the common form section limited to name, type, and enabled state for the primary visible layout
- [x] 1.3 Extend the device UI registry so type-specific sections can be resolved consistently for view, edit, and create modes
- [x] 1.4 Preserve readonly fallback behavior for device types that do not provide editable or create-specific sections

## 2. Dialog Integration

- [x] 2.1 Refactor `DeviceDetailDialog` to render existing-device View/Edit content through the shared Device form shell
- [x] 2.2 Refactor `DeviceCreateDialog` to render Create content through the shared Device form shell
- [x] 2.3 Ensure Create mode shows type-specific fields only after type selection and resets type defaults when the type changes
- [x] 2.4 Ensure Devices table and Dashboard entry points open the same shared form behavior

## 3. Type-Specific Form Sections

- [x] 3.1 Update Dummy device detail rendering so it stays compact and does not show an empty type-specific config section
- [x] 3.2 Split GPIO switch rendering into a primary section with `GPIO pin` and `Output state`
- [x] 3.3 Move GPIO switch `Startup state`, safe state / safe mode, `Restore previous state`, and `Inverted` into a collapsed-by-default `Config details` disclosure
- [x] 3.4 Keep GPIO switch quick commands visible outside `Config details` in View mode and disabled when commands are not allowed
- [x] 3.5 Ensure GPIO switch Create mode submits default secondary config values when `Config details` is not expanded or changed

## 4. Localization And Accessibility

- [x] 4.1 Add or update English and Russian localization keys for shared form labels, `Config details`, mode actions, GPIO secondary fields, and tooltip copy
- [x] 4.2 Add `i` tooltip icons for `Startup state`, `Safe state`, and `Restore previous state` in View, Edit, and Create modes
- [x] 4.3 Ensure icon-only dialog actions have accessible names or tooltips
- [x] 4.4 Verify the shared form remains usable on mobile dialog fullscreen layout

## 5. Theme System

- [x] 5.1 Centralize light and dark color roles in the Vuetify theme definitions so surfaces, text, borders, chips, and dialogs share semantic tokens
- [x] 5.2 Replace hard-coded surface and text colors in `main.css` with theme-aware variables where possible
- [ ] 5.3 Verify the app bar, navigation drawer, dialogs, cards, tables, and forms remain readable in both themes

## 6. Verification

- [ ] 6.1 Add or update frontend coverage for Dummy View mode common fields
- [ ] 6.2 Add or update frontend coverage for GPIO switch View mode primary fields, collapsed config details, tooltip icons, and quick commands
- [ ] 6.3 Add or update frontend coverage for Create mode type selection and GPIO switch defaults
- [ ] 6.4 Add or update frontend coverage for theme switching and semantic color readability in light and dark modes
- [ ] 6.5 Run `pnpm --dir portal-spa smoke` for SPA verification
- [ ] 6.6 Run `scripts/test.sh` for full local verification
