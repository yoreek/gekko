## 1. Dependency and Registry Setup

- [x] 1.1 Add `@mdi/js` to the SPA dependencies and refresh the lockfile
- [x] 1.2 Define the icon source split between mdi-backed action icons and local domain icons
- [x] 1.3 Update the shared icon registry data structure so one lookup path can resolve both sources
- [x] 1.4 Add an explicit curated list of mdi imports so only used icons are included

## 2. Icon Adapter Migration

- [x] 2.1 Refactor icon consumers to use Vuetify's standard `icon`/`VIcon` API with semantic names
- [x] 2.2 Refactor the Vuetify icon set adapter to consume the new registry format without changing call sites
- [x] 2.3 Keep the existing fallback icon behavior for unknown names
- [x] 2.4 Update any alias mappings so Vuetify and app-level icon names stay consistent

## 3. Usage Audit and Verification

- [x] 3.1 Replace routine UI icon usages with the new mdi-backed names where the registry already exposes them
- [x] 3.2 Verify icon-only controls, navigation items, and metric cards still render correctly after the migration
- [x] 3.3 Run the relevant project checks or UI smoke test that covers the updated icon layer
- [x] 3.4 Run the production build and confirm the bundle stays at or below 500 kB
- [x] 3.5 Add a small fixed manual chunking strategy to keep the number of chunks low while removing the large-chunk warning
