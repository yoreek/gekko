## Context

The portal SPA already uses Vuetify, a local icon registry, and a theme toggle, but the visual system is still spread across `vuetify.ts`, `main.css`, and component-local overrides. That makes the current look difficult to balance and makes future changes risky, because each screen can drift from the shared style.

The desired direction is a CoreUI-like operational UI: compact, readable, with visible surface hierarchy, borders, and consistent component treatment. The design should not introduce a separate design tool or a CSS framework. The application should continue to use Vuetify components as the base rendering system.

## Goals / Non-Goals

**Goals:**
- Create one global source of truth for portal theme colors, component defaults, and icon aliases.
- Make surfaces, borders, and text readable in both light and dark modes without per-component color hacks.
- Reduce the need for local style overrides by moving standard shape/density/variant settings into defaults.
- Keep the local icon registry as the only icon source for the app shell and Vuetify aliases.
- Preserve the current Vuetify-first component model.

**Non-Goals:**
- Replacing Vuetify with another UI library.
- Building a theme editor or configuration UI for end users.
- Redesigning portal workflows or business behavior.
- Turning the app into a pixel-perfect CoreUI clone.
- Moving every style decision into CSS if Vuetify defaults already express it cleanly.

## Decisions

- Use a dedicated global portal theme module as the single place for theme definitions, component defaults, and icon aliases. This is preferable to keeping theme state inline in `vuetify.ts`, because it gives one place to tune the base look.
- Keep Vuetify components as the base UI primitives and apply defaults to all standard Vuetify components used by the app. This is preferable to custom markup because it preserves keyboard, accessibility, and interaction behavior already provided by Vuetify while keeping appearance consistent across the whole UI.
- Use one shared card shell treatment for device cards, dashboard widgets, and other similar compact surfaces. This is preferable to letting each card-like component define its own radius or surface treatment because it removes visible drift in the UI language.
- Remove local styling and ad hoc visual props when the same outcome is expressible through global Vuetify defaults or theme tokens. This keeps component templates focused on semantics and reduces duplicated visual decisions.
- Use semantic theme roles and CSS variables for shell-level surfaces, borders, and shadows. This is preferable to hard-coded component colors because the same tokens can be reused in light and dark mode.
- Keep `main.css` focused on layout, spacing, and shell-level tokens. This is preferable to component-scoped color overrides because it keeps the base visual system centralized.
- Register the local icon set as the default icon source for Vuetify aliases. This is preferable to adding a separate icon package because the app is bundle-sensitive and already maintains a local registry.
- Allow local overrides only for true semantic exceptions such as status-specific widget states, not for the base shape or surface treatment. This keeps the global system authoritative.
- Keep the remaining CSS in `main.css` limited to shared shell exceptions that represent the portal foundation itself: app shell, page surfaces, dialog shells, dashboard widgets, device cards, tables, empty states, and other cross-screen primitives. Component-scoped style blocks should remain reserved for intrinsic SVG/icon rendering or genuinely local layout edge cases.

## Risks / Trade-offs

- [Risk] Over-centralization can make unique screens harder to tune. → Mitigation: allow scoped exceptions only when a component has a documented semantic reason.
- [Risk] Changing the shared defaults can affect many screens at once. → Mitigation: validate the dashboard, devices table, dialogs, and forms together before shipping.
- [Risk] Custom icon wiring can fail if alias names drift. → Mitigation: keep aliases and icon names in one typed registry module.
- [Risk] The global style system may still need a small amount of layout CSS. → Mitigation: keep CSS limited to shell layout and surface tokens, not component behavior.

## Migration Plan

1. Extract the global portal theme foundation into a dedicated module.
2. Wire Vuetify defaults and icon aliases to that module.
3. Reduce `main.css` to shell layout and token-driven surface rules.
4. Remove or simplify component-local surface overrides where the global defaults now cover the case.
5. Verify the dashboard, devices page, dialogs, and forms in both light and dark themes.
6. If a screen still needs a special treatment, add the smallest possible scoped exception and keep the base defaults unchanged.

## Open Questions

None.
