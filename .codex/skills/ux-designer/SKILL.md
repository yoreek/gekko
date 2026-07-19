---
name: ux-designer
description: Vuetify-first UX review and layout guidance for SPA screens, forms, dialogs, tables, themes, spacing, and readability.
---

# UX Designer

Use this skill for UI review, layout cleanup, readability fixes, and compact form design in Vue/Vuetify SPAs.

## Core Rules

- Prefer standard Vuetify components, props, density, and theme tokens first.
- Keep one component per concern; avoid duplicating view/edit/create layouts when one mode-aware component is enough.
- Use `surface`, `background`, `on-surface`, and `on-surface-variant` tokens for text and container colors.
- Do not use custom colors unless a standard Vuetify token cannot express the state.
- Keep forms compact and consistent across modes; only the control state should change, not the overall structure.
- Preserve readability in both light and dark themes.
- Use tooltips or hint text only for fields that need explanation; avoid adding helper text everywhere.
- When the UI feels wrong, identify the specific spacing, contrast, or hierarchy problem first, then fix the minimum necessary surface.

## Review Approach

1. Inspect the existing Vuetify pattern already used in the screen.
2. Identify whether the issue is structure, density, contrast, or state handling.
3. Reuse an existing component before introducing a new one.
4. Keep layout stable between `view`, `edit`, and `create` modes.
5. Verify the result in a browser when the task changes visible UI.

## What to Optimize

- card/header hierarchy
- compact tables and readouts
- dialogs and forms
- stateful controls and hints
- table actions
- theme-safe text contrast

## What to Avoid

- custom CSS hacks that fight Vuetify defaults
- separate duplicated forms for the same entity
- low-contrast text on light backgrounds
- theme-specific hard-coded colors
- changing layout structure just because mode changes

