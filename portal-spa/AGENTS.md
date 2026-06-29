# Portal SPA Rules

## Related Skills

- [vuetify-vue-spa](~/.codex/skills/vuetify-vue-spa/SKILL.md)
- [vue-best-practices](~/.codex/skills/vue-best-practices/SKILL.md)

## Vue And Vuetify UI Rules

- Prefer standard Vuetify components, props, and slots before any custom markup or CSS.
- If Vuetify already provides the behavior or visual pattern, use it directly and do not reimplement it in local HTML/CSS.
- Do not add custom headers, icons, expand/collapse controls, status markers, or similar UI chrome when the Vuetify component already exposes them.
- Keep `src/styles/main.css` minimal: reset, layout, spacing, and structure only.
- Do not place colors, opacity, state styling, label styling, or component behavior overrides in `src/styles/main.css`.
- Use theme tokens and Vuetify defaults for color, contrast, labels, and surface styling.
- For expansion panels, use the standard Vuetify accordion behavior and built-in expand/collapse UI.
- If a UI change would deviate from a standard Vuetify pattern, state that explicitly before editing and get confirmation first.
- For icon registry work, follow [app-icon-registry](../openspec/specs/app-icon-registry/spec.md) so Vuetify aliases, local SVG icons, and fallback behavior stay consistent with the shared registry contract.

## Checks

- For SPA browser validation, use MCP Playwright only, against `http://127.0.0.1:5176/?mockMode=1&mockReset=1`.
- Do not use ad hoc browser probes for SPA validation.
- Keep the existing local verification flow for unit and smoke tests unless the user asks for a different procedure.

## Text And Font Layout

- Keep font handling backend-agnostic: define shared font contracts and text layout helpers before display-specific code.
- Support at least monospace and proportional/custom glyph metrics as separate implementations.
- Compute text width, wrapping, line count, and autosize from font metrics rather than from one fixed glyph advance.
- Keep backend-specific text adapters thin so OLED, TFT, and matrix renderers can share the same layout model.
