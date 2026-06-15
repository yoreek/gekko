## Why

The current `Device details` card is visually too loose: the form takes more vertical space than it should, the GPIO details can fall below the fold, and related field groups do not read as distinct sections. This makes the modal harder to scan and slows down device editing, especially on smaller screens.

## What Changes

- Tighten the `Device details` layout so the shared fields and GPIO-specific fields occupy less vertical space.
- Group related form fields into clearer visual sections so the card reads as structured content instead of one long list.
- Use the same compact section pattern for both `view` and `edit` so the modal feels like one consistent surface.
- Use the same compact section pattern for `view`, `edit`, and `create` so the modal feels like one consistent form.
- Add clearer section backgrounds and reduce padding so the boundaries between groups are visible without adding heavy chrome.
- Render `view` mode with the same input-shell geometry as `edit` and `create` mode, using `readonly` controls for values that are being displayed rather than edited.
- Make the GPIO details area compact enough that it is more likely to remain visible without scrolling on common laptop and desktop viewports.
- Preserve the existing device editing and viewing behavior while improving density, hierarchy, and readability.

## Capabilities

### New Capabilities

- `device-details-compact-layout`: compact grouping and presentation rules for the device detail modal surface.

### Modified Capabilities

- `device-dashboard-ui`: update the device detail modal surface requirements to favor one shared compact pattern for view, edit, and create, clearer field grouping, and sectioned presentation for device details, especially GPIO-specific content.

## Impact

- Portal SPA device detail modal and shared device form components.
- Type-specific GPIO switch detail rendering and section composition.
- Existing layout and typography tokens used by the portal UI.
- No backend API or firmware behavior changes are expected.
