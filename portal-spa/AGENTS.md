# Portal SPA Rules

## Related Skills

- [vuetify-vue-spa](~/.codex/skills/vuetify-vue-spa/SKILL.md)
- [vue-best-practices](~/.codex/skills/vue-best-practices/SKILL.md)

## Vue And Vuetify UI Rules

- Prefer standard Vuetify components, props, slots, density, variant, color, layout, and built-in utility classes before any custom markup or CSS.
- Treat handwritten styles as a critical error.
- Do not use `<style>` blocks or inline `style="..."` attributes for component layout or styling.
- Build layout as a flat structure: `section -> row -> col`. A `section` should contain a sequence of sibling `v-row` blocks, and each `v-row` should contain only `v-col` children.
- Do not nest a new `section` inside an existing `section` unless the content is a truly separate form region.
- Do not mix `section`, `div`, `row`, and `col` as ad hoc wrappers when the same structure can be expressed with standard Vuetify rows and columns.
- Do not apply `section -> row -> col` mechanically. Keep semantically related controls in the same row when they form one compact control cluster, and split into separate rows only when that improves readability or alignment.
- Build layout with `<v-container>` -> `<v-row>` -> `<v-col>` and use `ga-*`, `ma-*`, `pa-*`, `d-flex`, `flex-row`, `flex-column`, `justify-center`, and `align-center` instead of custom spacing or flex CSS.
- Use component props for `color`, `bg-color`, `rounded`, `elevation`, `density`, and `size`.
- Use responsive breakpoint props like `cols="12"` and `md="6"` directly on `v-col`.
- If Vuetify already provides the behavior or visual pattern, use it directly and do not reimplement it in local HTML/CSS.
- Do not add custom headers, icons, expand/collapse controls, status markers, or similar UI chrome when the Vuetify component already exposes them.
- Keep `src/styles/main.css` minimal: reset, layout, spacing, and structure only.
- Do not place colors, opacity, state styling, label styling, or component behavior overrides in `src/styles/main.css`.
- Use theme tokens and Vuetify defaults for color, contrast, labels, and surface styling.
- If a UI change would deviate from a standard Vuetify pattern, state that explicitly before editing and get confirmation first.
- For expansion panels, use the standard Vuetify accordion behavior and built-in expand/collapse UI.
- For icon registry work, follow the existing alias/SVG/fallback contract in `src/icons/index.ts` so new icons stay consistent with it.

## UX Principles

- Optimize for the primary task first; do not add controls, labels, or helper text that do not help the user complete the current action.
- Keep related controls together when they form one logical operation, and split them only when separation improves comprehension or prevents accidental interaction.
- Preserve a stable hierarchy across `view`, `edit`, and `create` modes; the mode should change state, not reshape the whole form.
- Keep forms compact by default, but never at the cost of overlap, clipped text, or ambiguous grouping.
- Use hints and helper text only when a field is not self-explanatory; keep them short enough to fit the available layout.
- Prefer visible, direct controls over hidden or delayed behavior when the action is important to understanding or debugging a device.
- Make status and diagnostics readable at a glance: use clear labels, short values, and consistent placement.
- Avoid forcing fields to stretch or compress in a way that breaks readability; let the layout breathe enough for text and controls to fit naturally.
- If a control cluster becomes visually noisy, simplify the grouping before adding new styling or custom spacing.

## Checks

- For SPA browser validation, use MCP Playwright only, against `http://127.0.0.1:5176/?mockMode=1&mockReset=1`.
- Do not use ad hoc browser probes for SPA validation.
- Keep the existing local verification flow for unit and smoke tests unless the user asks for a different procedure.

## Text And Font Layout

- Keep font handling backend-agnostic: define shared font contracts and text layout helpers before display-specific code.
- Support at least monospace and proportional/custom glyph metrics as separate implementations.
- Compute text width, wrapping, line count, and autosize from font metrics rather than from one fixed glyph advance.
- Keep backend-specific text adapters thin so OLED, TFT, and matrix renderers can share the same layout model.
