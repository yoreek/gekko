## Context

This change documents the last thermostat-fix commit in a repo-local OpenSpec change. The code path spans firmware config parsing, runtime output propagation, websocket state emission, and SPA edit/detail behavior.

The implementation already exists in the main tree. The design here records the technical intent so the change can be reviewed, archived, and traced back to the behavior it stabilized.

## Goals / Non-Goals

**Goals:**

- Preserve thermostat target, safe-range, and hysteresis config updates instead of silently falling back to defaults.
- Make thermostat-driven switch output changes visible through runtime dirty tracking and realtime snapshots.
- Keep the thermostat detail view compact and remove the low-value last-check row.
- Ensure thermostat edit Save follows the actual draft state instead of remaining disabled after real changes.

**Non-Goals:**

- Do not introduce a new thermostat algorithm.
- Do not change the thermostat device model to own the sensor or switch hardware.
- Do not redesign the dashboard layout beyond the thermostat card/detail adjustments already implemented.

## Decisions

### Accept both canonical and alias thermostat config fields

The firmware keeps canonical fixed-point storage but accepts the Celsius-friendly aliases that the SPA and older clients may submit.

Alternative considered: reject alias fields and force a single representation. That is stricter, but it would make the UI and existing update paths brittle for no behavioral gain.

### Treat output transitions as visibility events

Switch runtimes mark themselves dirty only when logical output actually changes. That keeps snapshots accurate without broadcasting redundant updates for repeated identical requests.

Alternative considered: emit a websocket update for every request. That would be noisier and would not distinguish real state changes from no-ops.

### Keep thermostat detail compact

The thermostat detail view now focuses on control-relevant state: mode, target, current temperature, desired output, and actual output. The last-check field was removed because it added noise without helping the operator understand control state.

Alternative considered: keep last-check metadata visible in the compact card. That would preserve more internal state, but it makes the detail view harder to scan and does not change control behavior.

### Compute Save availability from draft diff

The edit form now enables Save when the draft diverges from the persisted snapshot, including config and dependency selections.

Alternative considered: enable Save whenever edit mode is open. That would be simpler, but it hides whether the current values really changed and makes disabled buttons look like a bug.

## Risks / Trade-offs

- [Config alias acceptance] can hide malformed client input if validation becomes too permissive. Mitigation: keep canonical output and validate bounded numeric ranges.
- [Snapshot dirty marking] can create extra traffic if runtimes report non-semantic changes. Mitigation: only mark dirty on actual output transitions.
- [UI simplification] may reduce debugging detail for operators. Mitigation: keep the canonical snapshot in the backend and expose more detail only when it is actually useful.
