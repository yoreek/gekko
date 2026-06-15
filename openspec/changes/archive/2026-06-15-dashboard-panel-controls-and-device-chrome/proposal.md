## Why

The dashboard and device detail modal already work, but a few UI affordances are unclear: the active dashboard panel had only scroll-style arrows instead of explicit move controls, and the device detail modal did not visually distinguish view mode from edit mode. That made the interface harder to scan and easier to misread, especially once the empty-state chrome and type-specific section wrappers were tightened.

## What Changes

- Replace the dashboard tab-strip arrows with explicit left/right panel move controls that reorder the active panel and preserve the saved layout.
- Add a clear view/edit mode indicator in the device detail modal header so readonly view mode is visually distinct without reducing field readability.
- Keep the device detail modal surface compact while avoiding redundant type-specific wrapper chrome that does not add useful information.
- Preserve the existing modal and panel behavior while making the active controls and current mode easier to understand at a glance.

## Capabilities

### Modified Capabilities
- `device-dashboard-ui`: update the device detail modal presentation to include an explicit mode indicator and reduce redundant chrome around type-specific content.
- `panel-dashboard-ui`: update the dashboard panel UI to use explicit move controls for panel reordering in the active dashboard view.

## Impact

- Portal SPA dashboard header controls and active panel management.
- Portal SPA device detail modal header and type-specific detail composition.
- Shared i18n labels for view/edit mode indicators and panel move controls.
- Existing panel persistence and device modal behavior remain unchanged.
