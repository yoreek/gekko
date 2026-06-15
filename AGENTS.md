# Project Rules

## Structure

- Keep firmware-internal headers in `src/` next to their `.cpp` files.
- Use `include/` only for a public library API that must be consumed from outside this firmware project.
- Prefer one class per domain/responsibility and split growing domains into focused files before they become large dispatchers.
- Prefer plain C arrays for fixed-size scratch buffers when a wrapper type adds no value.

## Non-Blocking Firmware Flow

- Use cooperative `loop()` execution.
- Do not add long blocking operations to runtime flow.
- Compute the loop timestamp once at the application boundary and pass it to timing-aware managers as `tick(uint32_t now)`.
- Do not call `millis()` or `clock_.millis()` inside domain state handlers when a cooperative tick already provides `now`.
- Use `src/core/StateMachine.h` for multi-step asynchronous or retry-oriented flows.
- Keep hardware waits, WiFi retries, provisioning, OTA state, and portal workflows explicit as states when the flow grows beyond a simple immediate action.

## Memory And Buffers

- Prefer stack storage, static storage, or reused buffers over repeated heap allocation and release in hot paths.
- Avoid creating large temporary buffers for an entire runtime if the data is only needed briefly.
- Reuse existing storage when it is bounded and the ownership is clear.
- Prefer streaming or direct serialization over concatenating large payloads into intermediate strings.
- Copy data only when ownership or lifetime requires it.
- Use heap allocations only when they are bounded, justified, and preferably confined to startup or rare control-path setup.

## Debug And Logging

- Use the local debug layer from `src/debug/Debug.h`.
- Do not add direct `Serial.print`/`Serial.printf` logging in domain code.
- Add or reuse domain-specific debug flags in `platformio.ini`, for example `WITH_WIFI_NETWORK_MANAGER_DEBUG` or `WITH_PORTAL_SERVER_DEBUG`.
- Keep logging behind build flags so production firmware can disable noisy domains.

## Checks

- Run `scripts/test.sh` for local verification. It runs `scripts/check.sh` before `pio test -e native`.
- `scripts/check.sh` requires `clang-format` and `cppcheck`.
- Keep code formatted by `.clang-format`.
- Do not run or reopen `preview` servers for visual checks unless the user explicitly asks for them; browser validation through Playwright MCP is allowed only when explicitly requested or when the user has already approved a live browser check for the current task.

## UI Design Rules

- Define typography, colors, borders, radius, and surface behavior globally first.
- Do not add component-specific text styling when the same role can be expressed by the shared design system.
- Use semantic text roles for headings, labels, table headers, card titles, dialog titles, secondary text, and muted text.
- Avoid local CSS overrides for `color`, `font-weight`, `letter-spacing`, `border-radius`, or `opacity` unless the element is a documented exception.
- Prefer Vuetify defaults, theme tokens, and shared classes over per-screen styling.
- Keep component markup semantic and minimal; do not invent one-off styles for names, titles, or labels that should follow the global typography layer.
- When describing UI work, specify the global rule, the forbidden local overrides, the scope, and the definition of done.

## Vue And Vuetify UI Rules

- Prefer standard Vuetify components, props, and slots before any custom markup or CSS.
- If Vuetify already provides the behavior or visual pattern, use it directly and do not reimplement it in local HTML/CSS.
- Do not add custom headers, icons, expand/collapse controls, status markers, or similar UI chrome when the Vuetify component already exposes them.
- Keep `portal-spa/src/styles/main.css` minimal: reset, layout, spacing, and structure only.
- Do not place colors, opacity, state styling, label styling, or component behavior overrides in `main.css`.
- Use theme tokens and Vuetify defaults for color, contrast, labels, and surface styling.
- For expansion panels, use the standard Vuetify accordion behavior and built-in expand/collapse UI.
- If a UI change would deviate from a standard Vuetify pattern, state that explicitly before editing and get confirmation first.
