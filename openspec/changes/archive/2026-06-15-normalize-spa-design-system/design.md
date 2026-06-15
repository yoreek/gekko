## Context

The current SPA design layer is not cleanly separated:

- `portal-spa/src/theme/portal-ui.ts` defines Vuetify themes/defaults, but many visual choices are still repeated in templates and `main.css`.
- `portal-spa/src/styles/main.css` contains layout rules, shell surface rules, component-specific typography, local text colors, local font weights, local letter spacing, local border radius, shadows, and state-like styling in one file.
- Components and views still pass visual props such as `variant`, `color`, `size`, `elevation`, `flat`, and `density` in places where those props are visual defaults rather than semantic state.
- Some texts are plain HTML tags inside custom containers. They do not consistently inherit a global typography role, so fixing readability has been happening through local selectors such as table-specific names, dialog sublines, metric labels, and empty-state copies.

This change treats the problem as design-system normalization, not as another color pass.

## Goals / Non-Goals

**Goals:**

- Establish one global source for typography roles, semantic colors, surfaces, radius, shadows, and Vuetify component defaults.
- Make normal markup render correctly by default: table headers, row names, card titles, dialog headings, labels, values, helper text, and empty-state text should not need component-specific CSS just to be readable.
- Make forms and fields consistent across pages, dialogs, view/edit/create modes, and custom field wrappers.
- Remove component-specific visual classes and props when they only duplicate global style decisions.
- Keep exceptions explicit and structural: layout, sizing, positioning, grid behavior, and documented semantic states are allowed.
- Preserve local icon registry usage and existing app behavior.

**Non-Goals:**

- Replacing Vuetify with another UI framework.
- Copying CoreUI code or CSS directly.
- Changing firmware APIs, data contracts, device behavior, or routing.
- Polishing every screen to a final visual brand in this change; the goal is a stable foundation that no longer fights itself.

## Decisions

### Decision: Global design roles first

Define shared roles before touching component details:

- text roles: body, title, label, secondary, muted, table header, value
- surface roles: page, surface, elevated surface, grouped surface, table surface
- shape roles: control radius, card radius, dialog radius
- depth roles: no elevation, low elevation, overlay/dialog elevation

Rationale: the same UI role appears across tables, cards, dialogs, and forms. A table row name and a card title must not require separate font-weight rules if they are the same semantic role.

Alternative considered: continue tuning selectors in `main.css`. Rejected because it recreates the current failure mode: each new screen exposes another low-contrast or inconsistent element.

### Decision: Vuetify defaults own component appearance

Use `portal-spa/src/theme/portal-ui.ts` and Vuetify defaults for standard components:

- inputs: `VTextField`, `VSelect`, `VAutocomplete`, `VCombobox`, `VTextarea`, `VSwitch`, `VField`, `VInput`
- actions: `VBtn`, `VBtnToggle`, `VChip`, `VTooltip`
- containers: `VCard`, `VDialog`, `VToolbar`, `VAppBar`, `VNavigationDrawer`, `VExpansionPanel`, `VTable`, `VSheet`, `VList`, `VListItem`, `VTabs`

Rationale: component geometry such as density, variant, rounded corners, elevation, and base color should be changed globally. Templates should only set semantic state, such as `color="error"` for destructive actions or `loading` for async actions.

Alternative considered: global CSS overrides for Vuetify internals. Rejected because it is brittle and bypasses Vuetify's theme/defaults system.

### Decision: Forms and fields use one shared control system

All form surfaces and field presentations must resolve to one shared control system:

- editable controls use Vuetify input defaults first
- read-only values use a shared field/value role instead of per-dialog label classes
- custom field wrappers are allowed only when they provide structure or behavior that Vuetify controls do not provide
- repeated custom field wrappers must be extracted or normalized so label, value, hint, error, spacing, and alignment are consistent

Rationale: forms are where the current inconsistency is most visible. A field label in Create, Edit, View, Dashboard dialogs, and Device details must look like the same UI role, not like unrelated local styles.

Alternative considered: keep form fixes inside each dialog/component. Rejected because it duplicates label/value/hint styling and recreates the same readability problems.

### Decision: `main.css` becomes a shell and typography boundary, not a component skin

`main.css` may define:

- root typography variables and CSS custom properties sourced from Vuetify theme tokens
- app shell layout
- page layout primitives
- structural sizing/positioning that Vuetify props cannot express cleanly
- semantic role classes only when the role is shared across multiple contexts

`main.css` must not define one-off visual rules for a specific table, card, dialog, or screen when a global role or Vuetify default can express it.

Rationale: keeping all custom CSS out is unrealistic because the app has shell layout and custom dashboard grid behavior. The boundary must be explicit: layout is allowed, local visual skins are not.

### Decision: Global CSS vs scoped Vue CSS boundary

Global design-system rules belong in global CSS or Vuetify defaults when they must apply across the app. Examples: typography roles, shared text roles, page shell layout, shared surface variables, and global form/control defaults.

Scoped Vue CSS is allowed only for component-private structure that cannot be expressed cleanly through Vuetify props or shared layout classes. Examples: a component's internal grid placement, overflow behavior, fixed widget dimensions, or private positioning.

Scoped Vue CSS must not define a new visual language for colors, labels, headings, field spacing, radius, shadows, opacity, or repeated control appearance. If a scoped rule is needed in more than one component, it belongs in the global design layer or in a shared component.

Rationale: putting every style in scoped Vue files hides duplication and makes shared UI roles diverge. Putting every style in `main.css` creates global leakage. The boundary is role-based: shared visual roles are global; private structure is scoped.

### Decision: Semantic markup replaces component-specific title/name classes

Where a piece of text is a heading/name/title/value, markup should express that role directly through:

- standard headings when appropriate
- Vuetify typography utilities when appropriate
- shared semantic classes when a custom container needs a repeated role

Component-specific names such as `devices-table__name` must not exist solely to set font weight or text color.

Rationale: the user should not have to find every screen where the same text role was styled differently.

### Decision: Audit before implementation edits

Implementation must start with an audit output that classifies each visual override into:

- remove
- move to Vuetify defaults
- move to global semantic role
- keep as layout/structural exception
- keep as semantic state

Rationale: editing without classification caused regressions. The audit is the guardrail for implementation order.

## Risks / Trade-offs

- [Risk] Removing local visual rules may expose places that relied on accidental styling. → Mitigation: remove by category, not randomly; verify key screens after each category.
- [Risk] Some props are both visual and semantic depending on context. → Mitigation: keep props only when the component state is semantic, for example `color="error"` on delete or `variant="tonal"` for status chips.
- [Risk] Making every text role too strong reduces hierarchy. → Mitigation: define readable secondary/muted roles globally, then use them deliberately rather than local opacity/color patches.
- [Risk] The old `coreui-style-global-theme` change overlaps with this work. → Mitigation: treat that change as prior exploratory work; this change becomes the actionable normalization contract.

## Migration Plan

1. Produce the visual override audit and classification.
2. Normalize `portal-ui.ts` defaults for standard Vuetify components.
3. Reduce `main.css` to global roles, shell layout, and documented structural exceptions.
4. Replace component-specific visual classes with semantic markup or shared roles.
5. Remove visual props from templates unless they represent semantic state.
6. Run `pnpm --dir portal-spa build`.
7. Verify key screens by DOM/style inspection first, and browser preview only for final interaction/readability confirmation.

## Open Questions

None.
