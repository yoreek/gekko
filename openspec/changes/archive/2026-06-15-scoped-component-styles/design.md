## Context

The portal SPA uses Vuetify for most UI structure, but the styling implementation had grown into a mixed global stylesheet with both shared primitives and component-specific rules. That made it harder to tell which component owned a selector and increased the chance of accidental cross-screen effects.

## Goals / Non-Goals

**Goals:**
- Keep component-owned styles physically near the component that uses them.
- Reserve the global stylesheet for reset, theme tokens, app shell, and shared primitives.
- Preserve the existing Vuetify-first approach and avoid unnecessary custom CSS.

**Non-Goals:**
- Redesign the visual system.
- Change product behavior or API contracts.
- Introduce new styling dependencies or CSS frameworks.

## Decisions

- Use `<style scoped>` in component SFCs for selectors that belong to a single component.
  - Rationale: the component source then owns both markup and styling, which makes maintenance and review simpler.
  - Alternative considered: keep a large shared stylesheet with BEM names. Rejected because it obscures ownership and makes accidental coupling more likely.
- Keep `main.css` for true globals only.
  - Rationale: reset rules, theme tokens, and app shell primitives are cross-cutting and belong in one shared file.
  - Alternative considered: move everything into scoped blocks. Rejected because theme tokens and app-shell layout must remain globally available.
- Use `:deep()` only when a component must target Vuetify internals.
  - Rationale: this preserves scoped ownership while allowing controlled styling of generated markup.
  - Alternative considered: global overrides for Vuetify internals. Rejected because those are harder to audit and easier to break.

## Risks / Trade-offs

- More files contain style blocks, which increases file count -> Mitigation: keep each component style block small and focused.
- Scoped styles can require `:deep()` for generated Vuetify markup -> Mitigation: use it only for targeted internal selectors.
- Shared primitives may still accumulate in `main.css` -> Mitigation: keep a strict definition of "global" and review any new selector against it.

## Migration Plan

- Move component-owned selectors into the owning component SFCs.
- Leave shared primitives in `main.css`.
- Update the Vue/Vuetify guidance to enforce the new rule for future work.
- If a future change needs a shared primitive, add it to `main.css` explicitly instead of reusing a component selector.

## Open Questions

- Should additional shared primitives be extracted into a dedicated shared stylesheet if `main.css` grows again?
- Should the project add a lint or review checklist item that explicitly rejects non-global selectors in `main.css`?
