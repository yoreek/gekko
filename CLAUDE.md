# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Commands

### Firmware (C++ / PlatformIO)

```sh
scripts/test.sh               # lint + native unit tests (run before committing)
scripts/lint.sh               # clang-format dry-run + pio check (esp32dev + native)
pio run -e esp32dev           # compile firmware
pio run -e esp32dev -t upload # flash via serial
pio run -e esp32dev_ota -t upload  # flash via OTA (192.168.1.240)
pio run -t uploadfs           # upload LittleFS (data/) to device
pio test -e native            # run Unity unit tests on host
```

`scripts/lint.sh` requires `clang-format` and `pio` in PATH. Code must pass `clang-format --dry-run --Werror` with the project `.clang-format`.

### Frontend (Vue SPA — `portal-spa/`)

```sh
cd portal-spa
pnpm dev                      # dev server
pnpm build                    # TypeScript check + Vite build
pnpm deploy:data              # build + copy gzipped assets into git-tracked data/
pnpm test:unit                # Node.js unit tests
pnpm smoke                    # Playwright end-to-end tests
```

SPA browser validation uses Playwright only, against `http://127.0.0.1:5176/?mockMode=1&mockReset=1`. Do not open preview servers for visual checks unless explicitly asked.

## Architecture

### Firmware

`src/core/App` is the top-level coordinator. `App::begin()` initialises all services; `App::tick()` drives them cooperatively. The timestamp `now` is computed once per `tick()` and passed into each service as `tick(uint32_t now)`. Do not call `millis()` inside state or tick handlers.

Multi-step or retry-oriented flows use `src/core/StateMachine.h` — see `src/wifi/WifiManager` as the reference state machine.

**Service layers (all owned by `App`):**

| Layer | Key classes |
|---|---|
| Platform | `ArduinoClock`, `ArduinoWifiDriver`, `PreferencesConfigStorage` |
| Config | `ConfigStore` (WiFi/provisioning) |
| WiFi | `WifiManager` — owns all `WiFi.*` calls and BLE provisioning states |
| Device registry | `DeviceRegistry`, `DeviceRegistryStore`, `DeviceRetainedDataStore` |
| Display | `DisplayLayoutStore`, `DisplayRenderCoordinator` |
| Portal | `PortalServer`, `PortalWebSocketManager`, controllers under `src/portal/controllers/` |

**Device model:**

Each device family has a versioned binary config struct (`*DeviceConfigV*` with a `kMagic` marker), a `DeviceRuntimeBase`-derived runtime class, and a REST API adapter under `src/integrations/rest/`. The registry stores config as a bounded binary blob; runtime state is always separate from persisted config. See `docs/device-config-versioning.md` before touching any `*DeviceConfigV*` struct or binary codec.

**Portal HTTP layer:**

Controllers inherit `BaseController` and declare a `beforeChain()` with `RulesChain` hooks for preconditions. The base chain owns CORS/preflight (`beforeCorsOptions`) and JSON body parsing. Controllers respond with `{"success":true,...}` or `{"success":false,"code":"...","error":"..."}`. See `docs/controller-ruleschain.md`.

**Debug logging:**

Use `src/debug/Debug.h` macros only. Domain-specific flags (e.g. `WITH_WIFI_NETWORK_MANAGER_DEBUG`) are defined in `platformio.ini`. Never add `Serial.print` directly in domain code.

**Memory discipline:**

Prefer stack/static/reused buffers over heap allocation in hot paths. Avoid building large intermediate `String`s or heap vectors for data that can be streamed or serialised directly.

### Frontend (portal-spa)

Vue 3 + Vuetify 4 SPA. Uses `pnpm`. State is managed with Pinia. Real-time updates come over a WebSocket from the firmware.

**API contract:** All REST payloads are camelCase. Devices use the `{ record, config, runtime }` envelope. `record` holds identity (`id`, `typeName`, `configRevision`); `config` holds persisted settings including `name`, `enabled`, and `deps`; `runtime` holds live state. `registryRevision` belongs to the collection envelope, not to individual device objects. See `docs/rest-api-contract.md` for the full contract.

**UI rules:** Use Vuetify components, props, slots, and layout classes (`v-container`/`v-row`/`v-col`, `ma-*`, `pa-*`, `ga-*`, `d-flex`). Do not write `<style>` blocks or inline `style=` attributes. Do not override colors, opacity, or typography locally — use theme tokens. If a change would deviate from a standard Vuetify pattern, state that before editing and confirm first.

**Vuetify 4 props:** Use `density="comfortable"` instead of deprecated `dense` prop on `v-row`, `v-alert`, and other Vuetify components. Never use bare `dense` attribute — it triggers upgrade warnings and is not supported.

### Data folder

`data/` contains the gzipped LittleFS assets served by the firmware. It is updated by `pnpm deploy:data` in `portal-spa/`. The LittleFS partition is 500 KiB; `portal-spa/scripts/check-data-budget.mjs` enforces this limit. Run `pio run -t uploadfs` to push a refreshed `data/` to the device.

## Key Docs

- `docs/device-config-versioning.md` — binary config migration rules (mandatory read before touching versioned structs)
- `docs/rest-api-contract.md` — canonical REST contract; `portal-spa/src/api/contracts.ts` is the TypeScript mirror
- `docs/device-model-structures.md` — C++ and TypeScript model hierarchy
- `docs/state-machine-rules.md` — cooperative state machine patterns
- `docs/controller-ruleschain.md` — HTTP controller and hook pattern
- `docs/device-registry-persistence.md` — NVS storage layout and commit order
- `docs/frontend-deployment.md` — SPA build → data/ → LittleFS workflow
- `openspec/specs/app-icon-registry/spec.md` — Vuetify icon alias and SVG registry contract
