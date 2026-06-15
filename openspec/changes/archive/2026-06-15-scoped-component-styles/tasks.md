## 1. Audit and isolate component styles

- [x] 1.1 Identify selectors in `main.css` that belong to a single component or view
- [x] 1.2 Move component-owned selectors into the owning Vue SFC `style scoped` blocks
- [x] 1.3 Keep global reset, theme tokens, app shell, and shared primitives in `main.css`

## 2. Encapsulate shared shells

- [x] 2.1 Move reusable dialog shell styles into `DeviceDialogShell.vue`
- [x] 2.2 Move device widget styles into `DeviceWidgetBase.vue`
- [x] 2.3 Move device card, field, panel manager, and detail dialog styles into their owning components

## 3. Update portal pages and guidance

- [x] 3.1 Move page-local dashboard and devices view styles into their view files
- [x] 3.2 Update the Vue/Vuetify skill to require scoped component styles by default
- [x] 3.3 Verify `main.css` no longer contains component-owned selectors
