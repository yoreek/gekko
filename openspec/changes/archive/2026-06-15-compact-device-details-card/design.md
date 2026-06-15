## Context

`DeviceDetailDialog.vue` already composes the shared modal surface, common device fields, and type-specific details, but the current layout leaves too much vertical whitespace between logical groups. The GPIO switch section in `GpioSwitchDeviceForm.vue` is functionally correct, yet the field blocks do not read as a dense, clearly separated information card, so the modal can overflow the viewport before the important GPIO details are visible.

The implementation should stay within the portal SPA and reuse the current Vuetify-based component stack. No backend contract, firmware behavior, or device data model changes are needed.
The new direction is to make `view`, `edit`, and `create` share the same compact section system rather than maintaining separate visual rhythms for the three states.

## Goals / Non-Goals

**Goals:**
- Make the device detail modal visually denser without changing edit/view behavior.
- Make the device detail modal visually denser without changing view/edit/create behavior.
- Create clear visual boundaries between shared identity fields, primary GPIO fields, and secondary GPIO configuration.
- Keep important GPIO details available earlier in the viewport on common desktop and laptop screens.
- Use one compact section grammar for view, edit, and create so the modal feels like a single surface.
- Reuse existing modal, field, and type-specific components instead of introducing a parallel details system.

**Non-Goals:**
- No API changes.
- No firmware changes.
- No changes to device command semantics, edit validation, or saved device payloads.
- No redesign of the dashboard widget cards or table rows outside the detail modal surface.

## Decisions

1. Keep the modal shell in `DeviceDialogShell.vue` and adjust only the inner composition in `DeviceDetailDialog.vue`.
   - Rationale: the shell already owns the dialog chrome, title bar, footer, and sizing behavior. Changing the inner composition keeps the work localized and reduces the risk of regressions.
   - Alternatives considered:
     - Rebuilding the dialog shell with a new layout system. Rejected because it would duplicate behavior already centralized in the shell.
     - Using a global layout override in `main.css`. Rejected because the repo rules discourage component-specific styling in the shared stylesheet.

2. Introduce compact sectioning at the detail-card level rather than inflating `DeviceField` into a heavier component.
   - Rationale: `DeviceField` is a low-level atomic control. It should stay reusable for display and control modes, while section structure belongs to the dialog/form composition layer.
   - Alternatives considered:
     - Adding a `dense` flag to every field component. Rejected because it spreads presentation concerns across too many components for a layout-only change.
     - Replacing fields with custom tables or definition lists. Rejected because it would diverge from the existing Vuetify form patterns and make type-specific editors less consistent.

3. Keep primary GPIO data visible above secondary configuration and retain the existing disclosure pattern for less important settings.
   - Rationale: `GPIO pin` and `Output state` are the core values for scan-and-read usage. Secondary settings still belong in a collapsed disclosure, but the section itself should be visually explicit and compact.
   - Alternatives considered:
     - Expanding all GPIO config by default. Rejected because it worsens vertical height and pushes the modal further past the fold.
     - Hiding more fields behind additional nested disclosures. Rejected because it adds interaction cost and obscures configuration state.

4. Preserve the current two-column responsive grid for field placement, but reduce the empty vertical space around it.
   - Rationale: the existing grid already works across breakpoints. The problem is spacing and grouping, not column count.
   - Alternatives considered:
     - Switching to a single-column stacked layout. Rejected because it would increase height and make the modal feel longer.
    - Moving to a dense table-like layout. Rejected because the form contains both display and control modes and needs Vuetify form affordances.

5. Make section surfaces visible through subtle background blocks, not only spacing.
   - Rationale: the current complaint is that the page is a single flat plane. A light surface treatment gives each group a visual anchor without adding too much chrome.
   - Alternatives considered:
     - Using strong borders and heavy cards for every group. Rejected because it would make the modal feel busy.
     - Keeping a fully flat background and relying only on padding. Rejected because the sections would still be hard to scan.

6. Use the same field-label rhythm in view mode, edit mode, and create mode.
   - Rationale: labels that sit in or above the field edge create a tighter, more editable-feeling layout and reduce the vertical stack height.
   - Alternatives considered:
     - Keeping display-mode labels above every value while edit-mode labels stay inside controls. Rejected because it preserves the visual mismatch between states.
     - Converting display mode to raw text rows with no labels. Rejected because that hides structure and weakens accessibility.

7. Render view mode with the same input-shell geometry as edit mode and rely on `readonly` for display, while create mode reuses the same shell and swaps in editable controls.
   - Rationale: this keeps the modal visually consistent, preserves copy/select behavior, and makes labels line up with the surrounding edit fields.
   - Alternatives considered:
     - Using `disabled` for all view-mode controls. Rejected because it makes the display look inactive and typically reduces readability.
     - Using plain text rows in view mode. Rejected because it breaks the compact form rhythm that the change is aiming for.

## Risks / Trade-offs

- [Risk] Over-compressing the modal could make labels wrap more often on narrower screens -> Mitigation: keep the responsive two-column grid and let the small breakpoint fall back to a single column where needed.
- [Risk] Adding section boundaries could introduce extra visual chrome if overdone -> Mitigation: use a minimal set of section headers/dividers and keep the existing typography roles.
- [Risk] Type-specific components may drift in spacing if only the main dialog is updated -> Mitigation: apply the same sectioning pattern to `GpioSwitchDeviceForm.vue` and verify other registered detail components still fit the modal.
- [Risk] Making view and edit share the same visual pattern may require extra component reshaping -> Mitigation: keep the data flow unchanged and standardize only the wrappers, spacing, and label placement.
- [Risk] The modal may still scroll on very small viewports -> Mitigation: accept scrolling on compact screens, but make the first viewport more information-dense than today.

## Migration Plan

1. Update the modal composition in `DeviceDetailDialog.vue` to introduce compact section wrappers around shared fields and type-specific detail blocks.
2. Refactor `GpioSwitchDeviceForm.vue` to keep primary GPIO fields prominent and render secondary settings in a compact, clearly separated disclosure.
3. Verify that `DeviceCommonFields.vue`, `DeviceField.vue`, and the registered type-specific detail components still render correctly without changing their data contracts.
4. Check the detail modal at desktop and mobile breakpoints, then tune spacing only where the modal still feels too loose.
5. Roll back by removing the new section wrappers and returning to the previous composition. No migration data is required.

## Open Questions

- Should the compact section treatment also be applied to other type-specific detail components besides GPIO switch as they are added?
- Do we want the modal to use a more explicit card-like section container, or is a lighter section header plus divider treatment enough?
- Should the same compact presentation be reused in Create/Edit dialogs, or should the scope stay limited to the read/edit detail modal first?
