# TODO / Roadmap

Working notes on features discussed but not necessarily finished. Grouped by status.

## Done (landed on master)

- **`[[deprecated]]` legacy-config guard rollout** — every multi-version device family's old
  `*ConfigV<n>` structs are marked `[[deprecated]]` and confined to the decode/migrate path via
  `EWFM_LEGACY_CONFIG_USE_BEGIN/_END`; `-Werror=deprecated-declarations` (scoped to `src/`) stops
  new code using a legacy version. Dead legacy code deleted where present. See
  `docs/device-config-versioning.md`.
- **Temperature calibration UI** (commit `ea1bbdc`) — `SensorCalibrationDialog` computes
  `SensorFilterConfig` coefficients from reference readings (one-point offset / two-point
  factor+offset) for ntc_thermistor, ds18b20, htu21 (temp + humidity). Math inverts the current
  coefficients since the runtime only exposes the filtered value. No firmware change.
- **Schedule presets** (commit `4badbf1`) — up to 3 named schedule snapshots per
  `scheduled_analog_output`, stored as POD records on the `devdata` LittleFS partition
  (`/sap/<deviceId>/p<slot>.bin`), modeled on the dose journal. REST: `GET /api/schedulepresets/<id>`,
  `PUT/DELETE /api/schedulepresets/<id>/<slot>`. Apply is SPA-driven (loads points into the schedule
  draft, saves through the normal config path). Point parse/validate/serialize shared between the
  config codec and the preset controller.

## Kept as-is (decided, no action)

- **`tools/devicegen/check_config_versions.py`** — runs automatically on every commit
  (`.githooks/pre-commit` → `scripts/test.sh` line 15). Now redundant with the compiler gate
  (`-Werror=deprecated-declarations`) but kept as belt-and-suspenders reserve. Remove later by
  dropping that line in `scripts/test.sh` + the file, if the duplication is unwanted.

## Deferred (design explored, not started)

### User-authored custom widgets on existing devices  ← main open item

Goal: let the user add their OWN widget (their own code/template) to an already-created device,
with real **formatting and styling** (a plain `{placeholder}` text template is not enough).

Key facts:
- Vue/Vuetify components are compiled into the bundle; no runtime template compiler and Vuetify is
  not exposed → a user widget is a plain HTML/JS **island**, not native Vuetify. Visual consistency
  comes from injecting our **theme CSS variables** into the widget container (the widget author
  writes their own DOM and uses `var(--...)` theme tokens).
- **Data plane already exists**: metric placeholder catalog + live values over WebSocket + the
  command pipeline. No per-device-type firmware needed for data.

Two "rich" options (both need `devdata` storage + an SPA renderer — the irreducible floor):
1. **HTML + CSS + `data-` binding, no arbitrary JS** (recommended start): user writes styled markup;
   we fill `data-metric` elements with live values, wire `data-command` buttons to the command
   pipeline (with confirmation for destructive ones), inject theme vars. Rich look, safe, medium
   effort.
2. **Full JS widget + a thin `sdk` facade** (in-page): user writes JS and owns the DOM;
   `sdk = { device, onUpdate, metric(s), sendCommand, t, theme }`. Most powerful; code runs in-page
   so it is trusted (single-owner LAN portal). Guards: error boundary, module scope, no tokens on
   globals.

Storage when built: widget code bundles on **devdata** LittleFS `/uw/<widgetId>` (reusable, keyed by
widgetId not deviceId), served via a REST controller — nearly a copy of the schedule-preset
storage+controller, but variable-length code + per-widget size/count caps + free-space guard. The
tile→(deviceId, widgetId) binding rides in the existing dashboard-layout payload (no new storage).
Not in git `data/`, not NVS. Mock: store in the mock DB like presets.

Open forks to decide when resuming: option 1 vs 2; CSP `script-src blob:` for option 2's
blob-module import; the command allow-list + confirmation policy.

### Originally raised, then set aside
- Universal/multi-protocol devices and no-recompile plugins — judged low-value earlier; not pursued.
