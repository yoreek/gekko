# Portal Alerts & Notifications

How the portal SPA surfaces device problems (e.g. an empty dosing-pump container) and how to
plug new alert sources into the same pipeline.

## Two notification layers

The SPA deliberately separates two concerns:

| Layer | Store | UI | Lifetime |
|---|---|---|---|
| **Transient toasts** | `src/stores/notifications.ts` | `NotificationSnackbar.vue` (`v-snackbar-queue`, mounted once in `App.vue`) | Seconds; fire-and-forget |
| **Standing alerts** | `src/stores/deviceAlerts.ts` | `AlertsBell.vue` (bell + `v-badge` counter + `v-menu` list in the app bar) | While the underlying condition holds |

Toasts answer "something just happened" (device saved, command sent, an alert *appeared*).
Standing alerts answer "something is wrong right now" and disappear on their own once the
condition clears. There is **no read/unread state and no persistence** — the alert list is a
pure function of current device runtime, so it survives reloads, never goes stale, and never
needs dismissing.

## Data flow

```
firmware runtime ──WebSocket──▶ realtime bridge ──▶ deviceRegistry store (devices[].runtime)
                                                            │ reactive
                                                            ▼
                              DeviceUi.extractAlerts(device)  (per-type hook, device-ui-registry)
                                                            │
                                                            ▼
                              deviceAlerts store: getter activeAlerts (computed, sorted critical-first)
                                                            │
                                        ┌───────────────────┴───────────────────┐
                                        ▼                                       ▼
                              AlertsBell badge + menu               AlertsBell watcher → one-shot toast
                                                                    (on appearance and on warning→critical)
```

No firmware or REST changes are needed as long as the condition is already visible in the
device's runtime snapshot.

## The per-type hook: `extractAlerts`

`DeviceUi` (`src/components/devices/registry/device-ui-types.ts`) has an optional member:

```ts
readonly extractAlerts?: (device: DeviceRecord) => DeviceAlert[]

export interface DeviceAlert {
  kind: string                              // stable per-device alert kind, e.g. 'container'
  severity: 'warning' | 'critical'
  messageKey: string                        // i18n key — resolved by the UI, follows locale switches
  messageParams?: Record<string, unknown>
}
```

Rules:

- Must be a **pure read** of the passed record — no side effects, no `millis()`-style time reads.
- `kind` must be stable across recomputes: `${deviceId}:${kind}` identifies the alert, and the
  toast watcher uses it to detect appearance/escalation. One `kind` per independent condition.
- Return message **keys**, not translated strings, so the bell menu re-renders on locale change.
- Severity mapping: `critical` → error color, sorted first, red badge; `warning` → warning color.

Reference implementation — dosing pump in `device-ui-registry.ts`: maps
`runtime.output.container.status` (`'critical'` → `notifications.alerts.containerEmpty`,
`'warning'` → `notifications.alerts.containerLow` with `{ percent }`).

## The `deviceAlerts` store

`src/stores/deviceAlerts.ts` exposes:

- `activeAlerts: ActiveDeviceAlert[]` — getter over `deviceRegistry.devices`, enriched with
  `id` (`${deviceId}:${kind}`), `deviceId`, `deviceName`; critical entries sorted first.
- `hasCritical: boolean` — drives the badge color (error vs warning).

Because it is a getter over the registry store, WebSocket upserts propagate automatically.

## The bell (`src/components/layout/AlertsBell.vue`)

- Bell `v-btn` in the `App.vue` app bar; `v-badge` shows the active-alert count (hidden at 0).
- `v-menu` lists alerts: device name as title, translated message as subtitle, entry links to
  the device detail route (`/devices/:id`).
- A watcher diffs `activeAlerts` by `id` and fires **one** toast per alert appearance, plus one
  more if a `warning` escalates to `critical`. `immediate: true` means alerts present at page
  load also toast once — intentional, so a problem is announced when the portal is opened.

Periodic re-toasting while a condition persists is intentionally **not** done — the badge is the
persistent reminder. If out-of-app reminders are ever needed, prefer pushing the condition to an
external channel (e.g. expose it via MQTT/Home Assistant discovery) rather than repeating toasts;
alternatively the AlertsBell watcher is the single place where a re-toast interval would go.

## Adding an alert for a new device type

1. Make sure the condition is present in the type's runtime snapshot (`src/api/contracts.ts`).
2. Add message keys under `notifications.alerts.*` in `src/i18n/locales/en.ts` **and** `ru.ts`.
3. Implement `extractAlerts` on the type's `DeviceUi` entry in `device-ui-registry.ts`.
4. Seed a mock instance that triggers the alert in `src/mock/database.ts` (see below).

Nothing else — the store, bell, badge, and toasts pick the new alert up automatically.

## Testing with mock seeds

`?mockMode=1&mockReset=1` seeds two dosing pumps specifically for the alert pipeline:

- **Nopox** — empty container (`status: 'critical'`): red badge, critical toast, red tank on card.
- **Magnesium** — 8% left (`status: 'warning'`): warning list entry and toast.

Keep at least one seeded device per alert-emitting type in an alerting state so the bell, toast,
and card signalling can be exercised straight from a fresh mock load (see the mock-seed rule in
`CLAUDE.md`).

## Related UI conventions

- The dashboard card itself should signal an active alert **without changing its size** (no
  appearing/disappearing chips or extra rows — card heights must not jump when a status flips).
  The dosing pump does this by coloring its container-tank outline and percent text via theme
  tokens (`text-error` / `text-warning`).
- Alert i18n lives under `notifications.alerts.*`; transient-toast strings for CRUD feedback stay
  at the `notifications.*` top level.
