## Context

The dashboard already supports panel reordering and device detail editing, but two pieces of chrome were misleading: the dashboard arrows looked like scroll controls instead of panel move controls, and the device detail modal did not clearly signal whether the user was looking at a readonly view or an editable form. At the same time, the device detail modal had acquired wrapper text that did not add useful information.

The implementation stays inside the portal SPA and uses the existing Vuetify component stack. No backend API or firmware behavior changes are required.

## Goals / Non-Goals

**Goals:**
- Make the dashboard panel move controls explicit and easy to understand.
- Preserve the existing panel ordering and saved layout behavior.
- Make device view/edit mode obvious without making the fields harder to read.
- Remove redundant modal chrome that does not add useful information.

**Non-Goals:**
- No changes to the dashboard layout persistence format.
- No backend contract changes.
- No firmware changes.
- No redesign of the device form field system beyond the header chrome and wrapper cleanup.

## Decisions

1. Replace the tab-strip arrow affordance with explicit left/right move controls.
   - Rationale: the previous arrows read as horizontal scroll controls, which was misleading. Explicit move buttons match the actual behavior and make the control intent obvious.
   - Alternatives considered:
     - Keeping scroll-style arrows. Rejected because they suggest the wrong interaction.
     - Moving panel reorder into a menu. Rejected because it hides a primary dashboard action behind an extra click.

2. Keep panel order persistence in the existing store workflow.
   - Rationale: the UI change is only about the controls, not the data model. Reusing the existing `movePanel` flow keeps the behavior stable.
   - Alternatives considered:
     - Introducing a new reorder API. Rejected because it would add work without changing the interaction.

3. Show a small mode chip in the device detail header instead of decorating every field.
   - Rationale: the mode distinction belongs at the dialog level. A header chip is visible immediately, keeps the fields readable, and avoids repeating state markers throughout the form.
   - Alternatives considered:
     - Adding per-field color changes. Rejected because it risks lowering contrast and creating visual noise.
     - Relying only on readonly behavior. Rejected because the distinction is too subtle once edit and view use the same input shells.

4. Remove the `Type-specific details` wrapper when it does not add information.
   - Rationale: the wrapper repeated what the content already communicates and increased chrome without improving comprehension.
   - Alternatives considered:
     - Keeping a titled wrapper for all type-specific content. Rejected because it adds a redundant layer to the modal.

## Risks / Trade-offs

- [Risk] The header chip could feel like extra chrome if it is too prominent -> Mitigation: keep it compact and aligned with the existing title/status row.
- [Risk] Explicit move controls may take a little more header width -> Mitigation: keep the controls compact and adjacent to the tab strip.
- [Risk] Removing the type-specific wrapper could make the modal feel less sectioned -> Mitigation: retain the shared field grouping and only drop redundant text.

## Migration Plan

1. Update the dashboard header to use explicit move-left and move-right controls for the active panel.
2. Keep the reorder behavior wired through the existing store so persistence remains unchanged.
3. Add a compact `View` / `Edit` indicator to the device detail modal header.
4. Remove the redundant type-specific wrapper title from the device detail modal.
5. Verify the header controls still work with the active panel state and the detail modal still renders the same fields.
