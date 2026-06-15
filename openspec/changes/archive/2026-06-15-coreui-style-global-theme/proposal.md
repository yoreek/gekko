## Why

The portal UI is still split across theme tokens, component-local styling, and a large global stylesheet, which makes the visual system hard to change consistently. The goal is to centralize the base look and feel so the app can be restyled from one global foundation without patching individual components.

## What Changes

- Introduce a single global UI foundation for the SPA that owns theme tokens, component defaults, icon aliases, and base surface styling.
- Move common look-and-feel settings into one shared configuration instead of repeating them per component.
- Keep component-local styling only for genuine functional exceptions or semantic states.
- Unify card-like device surfaces so device names, dashboard widgets, and related cards share one consistent shell treatment.
- Remove custom style blocks and visual props from components wherever the global Vuetify defaults and shared theme already cover the case.
- Preserve the existing Vuetify-first approach and local icon registry.
- Make light and dark themes readable by using one semantic token set for surfaces, borders, and text roles.

## Capabilities

### New Capabilities
- `portal-ui-foundation`: shared global theme, defaults, and surface system for the portal SPA.

### Modified Capabilities
- None.

## Impact

- `portal-spa/src/plugins/vuetify.ts`
- `portal-spa/src/styles/main.css`
- `portal-spa/src/theme/*`
- shared app shell components, dialogs, cards, tables, forms, and widget shells
- local icon alias registration
