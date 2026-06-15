## Why

The portal SPA currently mixes Vuetify theme/defaults with broad `main.css` rules and component-specific visual classes. This makes text readability, heading weight, table styling, card surfaces, and button radius inconsistent, and fixes tend to become local patches instead of a stable design system.

## What Changes

- Introduce a global SPA design-system contract for typography, text roles, surfaces, borders, radius, and Vuetify component defaults.
- Unify all forms and fields so input controls, labels, hints, read-only values, validation/error states, and custom field wrappers share one visual system.
- Audit and remove component-specific visual overrides such as table-only name/title styling, one-off label colors, local font weights, local letter spacing, local opacity, and local border radius.
- Keep component markup semantic and minimal: pages render normal Vuetify components and semantic tags, while shared appearance comes from the global design layer.
- Remove duplicated one-off custom field styling when the same form/field pattern appears in multiple places.
- Preserve local CSS only for layout, sizing, positioning, and documented structural exceptions that Vuetify defaults cannot express cleanly.
- Keep local icon registry usage and avoid adding UI/icon dependencies.

## Capabilities

### New Capabilities
- `portal-ui-design-system`: Defines the global SPA design system, typography roles, Vuetify defaults, CSS boundaries, and audit requirements for visual consistency.

### Modified Capabilities
- `portal-web-app`: Clarifies that existing theme colors must be applied through a global design system rather than per-screen component styling.

## Impact

- Affected code: `portal-spa/src/theme/**`, `portal-spa/src/plugins/vuetify.ts`, `portal-spa/src/styles/main.css`, `portal-spa/src/App.vue`, `portal-spa/src/views/**`, and shared UI components under `portal-spa/src/components/**`.
- No firmware API changes.
- No new runtime dependencies.
- Build verification remains `pnpm --dir portal-spa build`.
