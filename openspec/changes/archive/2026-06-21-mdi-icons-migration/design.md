## Context

The SPA now centralizes icon rendering through a single semantic icon registry and Vuetify's standard icon props. That gives us a clean place to change icon sourcing without touching every component. The main constraint is to keep the existing public icon names stable so the UI remains readable while we swap the underlying implementation for common action icons.

## Goals / Non-Goals

**Goals:**
- Replace routine UI/action SVG paths with `@mdi/js` imports.
- Preserve the current semantic icon names used by Vuetify props and existing views.
- Keep project-specific visuals in local SVG form when they represent product/domain concepts.
- Make fallback behavior deterministic for unknown icon names.
- Limit the migration to the icon layer and avoid unrelated UI changes.

**Non-Goals:**
- Redesigning the icon set or changing visible icon semantics.
- Replacing all custom domain icons with Material Design Icons.
- Changing layout, typography, colors, or component structure outside the icon abstraction.
- Introducing a second general-purpose icon package beyond `@mdi/js`.

## Decisions

### Use Vuetify's standard icon props directly
The application should use `v-btn` and `v-icon` with semantic icon names, while the registry decides whether each name resolves to a local SVG or an `@mdi/js` path. This keeps templates simple and makes it straightforward to remove custom icon components entirely.

Alternative considered: keep a custom wrapper component around every icon. Rejected because it adds an extra abstraction that the standardized Vuetify icon API no longer needs.

### Use mixed icon sources by category
Standard UI/action icons will be mapped to `@mdi/js` exports. Domain icons will remain in the local registry because they are project-specific and not guaranteed to exist in Material Design Icons with the same meaning or visual intent.

Alternative considered: move everything to `@mdi/js`. Rejected because several project icons are semantic brand/domain markers, and forcing them into a generic set would reduce clarity.

### Preserve existing icon names where possible
The registry should continue to expose the current icon names so the rest of the app does not need to learn a new naming scheme. The implementation changes the underlying source, not the calling convention.

Alternative considered: rename icons to match mdi naming. Rejected because it would create unnecessary churn across the app for no user-visible gain.

### Keep fallback behavior explicit
Unknown icon requests should resolve to the portal icon rather than throwing or rendering nothing. That keeps the app resilient during migration and makes missing mappings easy to spot visually.

Alternative considered: fail hard on unknown names. Rejected because it is too disruptive for a UI layer and would complicate migration.

### Add `@mdi/js` as a regular dependency
The package is a direct runtime dependency of the SPA and should be added explicitly. Only the imported icons will be included in the bundle, so the cost stays bounded.

Alternative considered: copy SVG paths into the repository manually. Rejected because it recreates maintenance work that the dependency already solves.

### Import mdi icons individually
The implementation should import `@mdi/js` symbols by name and only for icons actually used by the app. That preserves tree-shaking and keeps the dependency cost proportional to the number of selected icons.

Alternative considered: import the whole mdi set or build a dynamic registry from the package namespace. Rejected because it would weaken tree-shaking and risk bundle growth past the 500 kB cap.

## Risks / Trade-offs

- [Risk] Mapping mistakes could change icon meaning or visual consistency. -> Mitigation: keep a clear source map and verify key views after migration.
- [Risk] Adding `@mdi/js` increases dependency surface. -> Mitigation: import only selected icons and avoid wildcard icon sets.
- [Risk] Some Vuetify icon props may expect names not covered by the initial map. -> Mitigation: audit icon usages before removing local paths and keep fallback coverage.
- [Risk] A mixed registry can be confusing for future maintenance. -> Mitigation: document which names are domain icons versus standard UI icons and keep the registry centralized.
- [Risk] Pulling in too many mdi symbols could push the bundle over the 500 kB limit. -> Mitigation: keep a curated icon list, use named imports only, and verify bundle size after build.
- [Risk] Over-fragmenting the bundle could make startup and caching less predictable. -> Mitigation: keep manual chunking to a small fixed set of vendor groups.

## Migration Plan

1. Add `@mdi/js` to the SPA dependency list.
2. Replace the registry implementation so it can resolve either mdi path definitions or local SVG definitions from one source map.
3. Keep the Vuetify icon set adapter stable while updating its lookup logic to use the new registry format.
4. Map standard action icons to mdi and leave domain icons in the local SVG registry.
5. Use a small, stable manual chunking strategy to split the app into a few vendor chunks rather than many fine-grained chunks.
6. Run focused UI verification on the views and controls that use icon-only or icon-heavy buttons.
7. Run the production build and verify the resulting chunking removes the large-chunk warning while keeping the number of chunks low.
8. If any icon is visually wrong or missing, update the source map rather than reintroducing ad hoc local SVGs.

Rollback strategy:
- Remove the `@mdi/js` dependency.
- Restore the previous local SVG registry shape.
- Keep the same icon names so rollback does not require call-site changes.

## Open Questions

- Which exact icon names should remain local domain icons versus move to mdi for the first pass?
- Do we want to keep the current semantic names exactly as-is, or split some names into a more explicit domain/action namespace later?
