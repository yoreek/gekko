## Context

The project already has bus-owned runtime patterns for OneWire, I2C, and SPI. I2C and SPI runtimes currently expose only basic lifecycle state plus a generation token and transaction lock flag, which is enough for mutual exclusion but not enough for field diagnostics. OneWire already has scan support, and DS18B20 already demonstrates a useful pattern for transient error counters, but the new bus diagnostics change should not copy that logic into every bus class.

The key constraint is that diagnostics must stay runtime-only. They should not be serialized into persisted config blobs, setup bundles, or migration code. The runtime snapshot is the right place to surface them to the portal UI and WebSocket, and the existing realtime snapshot system already publishes full device snapshots when a runtime is marked dirty.

The scope for this change is shared bus runtimes only:
- I2C gets on-demand cooperative scan support plus diagnostics.
- SPI gets the same diagnostics envelope but no discovery/scan behavior.
- OneWire stays out of scope because it already has a separate scan model and different bus semantics.

## Goals / Non-Goals

**Goals:**
- Add one shared runtime diagnostics shape for bus-backed devices.
- Keep diagnostics runtime-only and resettable.
- Debounce diagnostic-only publication so repeated low-level errors do not flood the portal.
- Add cooperative, explicit I2C scan that steps one address per tick.
- Reuse the same diagnostics envelope for SPI without introducing SPI discovery.
- Expose the new runtime data through REST, WebSocket, and the bus detail UI.

**Non-Goals:**
- Persist diagnostics in config or retained state.
- Add generic hardware probing for every future peripheral protocol.
- Add SPI address discovery or a generic cross-bus scan abstraction.
- Change OneWire scanning behavior.
- Introduce per-device historical error storage or a ring buffer of past errors.

## Decisions

### 1. Use composition for the shared diagnostics helper instead of pushing everything into `DeviceRuntimeBase`
The shared error counters and publish scheduling should live in a small bus-focused helper that I2C and SPI bus runtimes compose, rather than making `DeviceRuntimeBase` responsible for bus-specific diagnostics policy. That keeps the base runtime focused on lifecycle/dependency mechanics and avoids growing a generic class with bus policy that only applies to a subset of devices.

Alternative considered: add diagnostics fields directly to `DeviceRuntimeBase`. Rejected because it would make the base class carry bus-only policy and add state to every runtime, even ones that never use it.

### 2. Keep diagnostics runtime-only and expose them through nested snapshot JSON
Diagnostics belong under `runtime.diagnostics` so they remain visibly separate from persisted `config` and from primary lifecycle `status`. This also makes the UI simpler: the portal can render a dedicated diagnostics section without mixing it into editable config fields.

Alternative considered: flatten the counters onto the top-level runtime object. Rejected because it makes the runtime payload noisy and harder to extend.

### 3. Add explicit `resetDiagnostics` as a named structured command
The reset action should be first-class, like other named runtime commands, instead of being hidden behind a generic custom payload. That keeps the command path consistent with the normalized API contract and makes the UI intent obvious.

Alternative considered: piggyback on a generic `custom` command string. Rejected because the project has already moved toward named runtime commands and the reset action is a stable public operation.

### 4. Debounce publication, not counter updates
Counters should update immediately in memory on every error, but publishing to the portal should be debounced when only diagnostic values changed. This keeps the UI accurate without forcing a WebSocket message for every transient error burst.

Alternative considered: throttle only the frontend or only the WebSocket layer. Rejected because the backend should own the visible publication cadence and keep REST snapshot semantics aligned with realtime snapshots.

### 5. Model I2C scan as a cooperative state machine with one address per tick
The scan is explicitly user-triggered and should advance one address per tick to keep the loop responsive and avoid blocking the bus layer for the whole address range. Using the existing transaction primitives and `endTransmission()` ACK result keeps the implementation simple and maps directly to the hardware behavior that actually matters.

Alternative considered: do the full sweep in one command handler or use keepalive pings. Rejected because full sweeps can monopolize the loop and keepalive traffic would add unnecessary bus noise.

### 6. Leave SPI without discovery and reuse only the diagnostics envelope
SPI is selected by chip-select, not by address, so there is no meaningful bus-level scan to add. The shared diagnostics fields are still useful because they show whether the SPI bus itself is repeatedly failing, but actual device existence checks remain device-specific `probe()` logic where a peripheral knows how to read its identity register.

Alternative considered: invent a generic SPI scan. Rejected because it would not map cleanly to the protocol and would create misleading expectations in the UI.

### 7. Publish scan completion and reset immediately, but coalesce error bursts
The user needs immediate feedback when a scan finishes or diagnostics are reset, but not every error increment needs its own broadcast. Immediate publish for scan completion/reset and debounced publish for diagnostic-only bursts gives the right balance between responsiveness and noise.

Alternative considered: make all runtime changes immediate. Rejected because repeated failures on a bad bus could otherwise create unnecessary realtime traffic and UI churn.

## Risks / Trade-offs

- [Risk] A shared helper plus nested diagnostics requires touching snapshot serialization in multiple places. -> Mitigation: keep the payload shape narrow and reuse the same helper in REST and realtime adapters.
- [Risk] Debounced publication can make very rapid fault toggling less visible if the interval is too long. -> Mitigation: keep the debounce short and publish immediately for status transitions, scans, and reset.
- [Risk] I2C scan one-address-per-tick will take longer than a single blocking sweep. -> Mitigation: the scan is explicit and user-triggered, so responsiveness matters more than raw completion speed.
- [Risk] SPI diagnostics may be mistaken for per-device presence checks. -> Mitigation: keep the UI wording explicit that SPI diagnostics describe the bus runtime, not a discovered device list.
- [Risk] Adding a named reset command expands the public API surface. -> Mitigation: keep the command scoped to supported bus runtimes only and make it purely runtime-only.

## Migration Plan

1. Add the shared bus diagnostics helper and the nested runtime snapshot fields.
2. Wire diagnostics serialization into the bus API adapters and realtime snapshot payloads.
3. Add `resetDiagnostics` command handling for supported bus runtimes.
4. Implement I2C scan as a cooperative state machine that stores scan results in runtime state.
5. Update the portal SPA bus detail views to render diagnostics and scan results, plus the reset action.
6. Add tests for diagnostics reset, debounce behavior, I2C scan progression, snapshot serialization, and realtime merge behavior.
7. Rollback is straightforward: remove the helper, reset command, and runtime snapshot fields. Because diagnostics are runtime-only, no config migration is required.

## Open Questions

- Should the initial debounce window be fixed in code or exposed as a compile-time constant? The design leans toward a fixed small constant to keep behavior deterministic.
- Should the reset action be available only from the UI, or also from generic API clients? The current design assumes both because the API already exposes structured device commands.
- Do we want to reuse the same reset command name for any future bus runtime, or keep it scoped to the bus diagnostics capability only? The current design keeps the name generic enough for reuse.
