# Project Rules

See also: [portal-spa/AGENTS.md](portal-spa/AGENTS.md)

## Structure

- Keep firmware-internal headers in `src/` next to their `.cpp` files.
- Use `include/` only for a public library API that must be consumed from outside this firmware project.
- Prefer one class per domain/responsibility and split growing domains into focused files before they become large dispatchers.
- Prefer plain C arrays for fixed-size scratch buffers when a wrapper type adds no value.

## Shared Architecture And Duplication

- Treat duplicated behavior, markup, validation, serialization, translations, and tests as an architecture defect, not as an acceptable type-specific implementation.
- Before adding a second implementation, identify the shared responsibility and extract it into one common class, component, service, model, adapter, or contract.
- Differences that are data, dimensions, capabilities, registration metadata, or hardware descriptors must be represented as configuration/profile data. Do not encode them as copied type-specific branches.
- Shared user-facing behavior must use short generic translation keys; loading, saving, success, and error messages must not repeat the current form or entity name when context is clear. Add type-specific translations only when the displayed meaning is genuinely different.
- Validation messages must be short and generic: do not create per-field variants or repeat the field name when context is clear; if a name is necessary, pass it as a placeholder to one shared message.
- Minimize code and bundle size: never add code, translations, messages, abstractions, or type-specific variants without necessity; reuse or simplify existing generic behavior first.
- A new type-specific file or branch requires a concrete justification describing the behavior that cannot be represented by the shared abstraction.

## Display Family Architecture

- LCD1602 and LCD2004 are one HD44780 character-display family. They must use one shared firmware base class, one shared REST adapter, one shared SPA model, one shared form, one shared preview, one shared designer contract, and one shared translation namespace.
- LCD1602 and LCD2004 differ only by registered type identity and logical geometry: LCD1602 is `16 x 2`, LCD2004 is `20 x 4`. Do not duplicate behavior, markup, validation, serialization, or translations for these two types.
- Display-specific differences belong in a profile or metadata object such as columns, rows, supported rotation, and hardware descriptor. They must not be implemented as copied `if lcd1602` / `if lcd2004` branches when the behavior is shared.
- All displays use the shared `layout` model. LCD must use only the `Character` widget; TM1637 must use only the `Digital` widget. LCD and TM1637 must not have line fields or a line-based REST/runtime model.
- LCD coordinates are character-cell coordinates. TM1637 coordinates are digit-cell coordinates. Pixel coordinates, font size, color, stroke, autosize, bitmap, and pixel-style flags must not appear in LCD/TM layout contracts.
- Hardware configuration and layout configuration are separate. Hardware pin/dependency fields remain in the common display form's hardware section; layout editing remains in the shared designer.
- User-facing labels, errors, hints, and validation messages for the shared display family must use common i18n keys. Do not add duplicated `lcd1602` and `lcd2004` translations for identical text.

## Duplication Stop Gate

- Before editing a display family, identify the shared class, model, adapter, form, preview, designer contract, and translation namespace.
- Before creating a second implementation or a type-specific branch, prove that the behavior is genuinely different. If it is only geometry or registration metadata, extend the shared profile instead.
- Stop the implementation if the proposed change introduces duplicated LCD1602/LCD2004 code, duplicated translations, duplicated REST handling, or legacy per-row fields. Refactor the common layer first.
- At the start of display work, state the common layer and the exact per-type differences before making file edits.

## Git Index Ownership

- The user owns any files that were already staged before the current task; do not unstage, reset, or overwrite that selection.
- When asked to commit, stage the files that belong to the current task, preserve unrelated staged and unstaged changes, and run `git commit -m "..."`.
- The pre-commit hook is expected to regenerate and stage `data/` and `webflash/`. Treat those hook-created index changes as part of the same commit attempt.
- If the pre-commit hook fails after changing the index, diagnose the failure, make the required in-scope fix, and retry the commit without treating the hook-created staging as a new user decision.
- Do not use `git reset`, `git restore --staged`, or otherwise remove staged changes unless the user explicitly asks.

## Mandatory Commit Gate

Before every `git commit`:

1. Read this `AGENTS.md` and [Frontend Deployment](docs/frontend-deployment.md) completely, even if they were read earlier in the conversation.
2. State in commentary that both files were read and that the mandatory commit gate is being followed.
3. Start exactly one `scripts/commit.sh "<message>"` process and retain its session ID if the command yields; never invoke `git commit` directly.
4. Poll that same session until it returns a terminal exit code. A yield, truncated output, or partial hook output is not commit completion.
5. Never start another commit while the previous commit process has an unknown or non-terminal status.
6. Never use `--no-verify`, and never use `--amend` to bypass or repair a pre-commit hook run.
7. If the hook aborts, follow the recovery procedure in [Frontend Deployment](docs/frontend-deployment.md) while preserving the user's index ownership rules above.
8. The wrapper writes the complete hook output to `/tmp/gekko-commit.log`, reports stage changes, and tails the final 80 lines. If the original tool session cannot be recovered, inspect it with `scripts/commit.sh --status`; never start a replacement commit while its status is running or unknown.

## Device Config Versioning

- Follow [Device Config Versioning](docs/device-config-versioning.md) before changing persisted `*DeviceConfigV*` structs, binary config markers, REST config fields, or migration code.

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

- After every change, audit the complete diff against all project requirements and remove unnecessary code, duplication, messages, translations, and abstractions before reporting completion.
- Run `scripts/test.sh` for local verification. It runs `scripts/check.sh` before `pio test -e native`.
- `scripts/check.sh` requires `clang-format` and `cppcheck`.
- Keep code formatted by `.clang-format`.
- Do not run or reopen `preview` servers for visual checks unless the user explicitly asks for them; browser validation through Playwright MCP is allowed only when explicitly requested or when the user has already approved a live browser check for the current task.
- For SPA browser validation, use MCP Playwright only, against `http://127.0.0.1:5176/?mockMode=1&mockReset=1`.
- Live HTTP checks against the ESP32, including repeated `curl` requests to `http://192.168.1.249/` and `/api/devices`, should be run outside the sandbox with escalated execution when the user asks for device verification.
- Do not attribute successful or failed live `curl` checks to the sandbox unless the command itself was run under sandboxed execution.

## UI Design Rules

- Define typography, colors, borders, radius, and surface behavior globally first.
- Do not add component-specific text styling when the same role can be expressed by the shared design system.
- Use semantic text roles for headings, labels, table headers, card titles, dialog titles, secondary text, and muted text.
- Avoid local CSS overrides for `color`, `font-weight`, `letter-spacing`, `border-radius`, or `opacity` unless the element is a documented exception.
- Prefer Vuetify defaults, theme tokens, and shared classes over per-screen styling.
- Keep component markup semantic and minimal; do not invent one-off styles for names, titles, or labels that should follow the global typography layer.
- When describing UI work, specify the global rule, the forbidden local overrides, the scope, and the definition of done.

## Text And Font Layout

- Treat text rendering as a separate shared domain from any specific display backend.
- Model fonts through a base contract plus explicit implementations for monospace and proportional/custom fonts.
- Derive glyph width, line wrapping, autosize, and bounding-box calculations from font metrics, not from a single fixed character width.
- Keep text layout logic in shared classes or services before adding display-specific adapters for OLED, TFT, or matrix backends.

## Vue And Vuetify UI Rules

- Prefer standard Vuetify components, props, and slots before any custom markup or CSS.
- If Vuetify already provides the behavior or visual pattern, use it directly and do not reimplement it in local HTML/CSS.
- Do not add custom headers, icons, expand/collapse controls, status markers, or similar UI chrome when the Vuetify component already exposes them.
- Keep `portal-spa/src/styles/main.css` minimal: reset, layout, spacing, and structure only.
- Do not place colors, opacity, state styling, label styling, or component behavior overrides in `main.css`.
- Use theme tokens and Vuetify defaults for color, contrast, labels, and surface styling.
- For expansion panels, use the standard Vuetify accordion behavior and built-in expand/collapse UI.
- If a UI change would deviate from a standard Vuetify pattern, state that explicitly before editing and get confirmation first.
- For icon registry work, follow the existing alias/SVG/fallback contract in `portal-spa/src/icons/index.ts` so new icons stay consistent with it.
