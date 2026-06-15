## Why

The portal SPA mixed component-owned selectors with the global stylesheet, which made ownership unclear and increased the risk of style leakage across screens. This change formalizes a component-scoped styling model so local UI details stay close to the component that owns them.

## What Changes

- Move component-owned selectors out of `portal-spa/src/styles/main.css` and into `style scoped` blocks in the owning Vue components.
- Keep `portal-spa/src/styles/main.css` limited to global reset, theme tokens, app shell, and shared layout primitives.
- Encapsulate shared dialog shell styling in the reusable dialog shell component.
- Update the portal Vue/Vuetify guidance to require scoped component styles by default.

## Capabilities

### New Capabilities
- `component-scoped-styles`: defines the portal styling contract where component-owned styles live in scoped component blocks and global CSS is reserved for shared application-level rules.

### Modified Capabilities

## Impact

- Affected code: `portal-spa/src/App.vue`, view components, reusable device components, `portal-spa/src/styles/main.css`, and the Vue/Vuetify skill guidance.
- APIs: none.
- Dependencies: none.
- Systems: frontend styling architecture and review workflow.
