## Why

The SPA currently maintains a custom SVG icon registry for both domain icons and routine UI actions. That works, but it duplicates common icon shapes, makes maintenance harder, and forces us to hand-manage every standard action icon. Moving routine icons to `@mdi/js` reduces local icon code while keeping the existing project-specific visuals where they add value.

## What Changes

- Introduce a new icon sourcing scheme that uses `@mdi/js` for standard UI/action icons.
- Keep local SVG icons for domain-specific concepts such as portal, device, Wi-Fi, OTA, system, panel, and device-type visuals.
- Refactor the app icon registry so each icon name resolves from a single, explicit source.
- Preserve Vuetify integration so standard icon props and aliases continue to work through a single semantic icon registry.
- Update icon usage across the SPA to rely on the new source map instead of hard-coded local SVG paths for common UI icons.
- Keep `@mdi/js` usage bounded to explicitly imported icons so the bundle stays under the project limit.

## Capabilities

### New Capabilities
- `app-icon-registry`: The web app SHALL resolve icons through a mixed registry that maps standard UI icons to `@mdi/js` and project-specific icons to local SVG definitions.

### Modified Capabilities
- 

## Impact

- `portal-spa` icon registry, Vuetify icon set wiring, and icon sizing defaults.
- `package.json` dependency graph through addition of `@mdi/js`.
- Any view or component that currently depends on the local icon registry.
- Bundle size and maintainability of the front-end icon layer.
- Production bundle budget: the migration MUST preserve the 500 kB maximum bundle size.
