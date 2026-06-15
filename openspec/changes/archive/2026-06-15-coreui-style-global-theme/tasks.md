## 1. Global foundation

- [x] 1.1 Extract the portal theme tokens, defaults, and icon aliases into one shared module
- [x] 1.2 Wire Vuetify to use the shared theme module and local icon registry as the default icon source
- [x] 1.3 Add a lightweight preview override path for light/dark inspection without depending on persisted storage

## 2. Shared surface system

- [x] 2.1 Reduce `main.css` to shell layout, spacing, and token-driven surface rules
- [x] 2.2 Define consistent global surface rules for app bars, drawers, cards, dialogs, tables, and empty states
- [x] 2.3 Remove or simplify local color, border, and shadow overrides that duplicate the global foundation
- [x] 2.4 Remove redundant visual props from component templates when defaults already cover the appearance

## 3. Component alignment

- [x] 3.1 Align dashboard device cards and widgets with the shared card foundation
- [x] 3.2 Align forms, selects, switches, dialogs, and expansion panels with the shared defaults
- [x] 3.3 Verify local icon aliases still render correctly in shell controls and disclosure widgets
- [x] 3.4 Unify all card-like device surfaces to the same radius, border, and elevation treatment

## 4. Validation

- [x] 4.1 Check the dashboard, devices table, and device dialogs in light and dark themes
- [x] 4.2 Run the SPA build and confirm the resulting UI remains readable and internally consistent
- [x] 4.3 Review the remaining CSS and document any exception that must stay local
