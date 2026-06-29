## Context

Display text widgets now need to reference values from multiple source classes: device metrics, system metrics, and wifi metrics. The designer must let users build those references without hard-coding IDs by hand, while the firmware must render the layout cooperatively and keep refresh cadence bounded.

The current firmware already redraws display layouts by clearing the surface and walking widgets in order, so the main technical decision is how to surface structured placeholders and how to treat unresolved values. The selected behavior is soft-fail resolution: missing sources do not create hard dependencies and do not block rendering or deletion of devices.

## Goals / Non-Goals

**Goals:**
- Provide a structured placeholder model with explicit namespaces.
- Let the designer pick a source device and metric from a catalog instead of typing raw identifiers.
- Preserve literal text editing and bounded validation.
- Keep runtime rendering cooperative and full-layout based.
- Treat missing source devices or unavailable metrics as non-fatal.

**Non-Goals:**
- Add hard dependency edges between display layouts and source devices.
- Implement partial redraw tracking per widget region.
- Redesign non-display device configuration flows.
- Change existing device registry deletion rules.

## Decisions

- Use explicit namespaces (`dev`, `system`, `wifi`) rather than a generic free-form token syntax.
  - Rationale: the catalog can be generated, localized, and validated consistently.
  - Alternative considered: keep `{value}`-style generic bindings. Rejected because it does not scale to multiple source classes and leaves the designer with too little structure.

- Expose a metric catalog through the firmware API and reuse it in the SPA picker.
  - Rationale: one catalog keeps the picker and renderer aligned and avoids duplicating source-specific logic in the frontend.
  - Alternative considered: hard-code picker options in the UI. Rejected because it would drift from runtime capabilities and make future metrics harder to add.

- Keep unresolved placeholders soft-failing at render time.
  - Rationale: layouts stay valid when a source device disappears or is temporarily unavailable, and users do not lose their display config.
  - Alternative considered: hard dependency edges with delete blocking. Rejected for the first version because it complicates registry operations and is stricter than the user requirement.

- Reuse the existing display render session to drive full redraws on refresh.
  - Rationale: the current firmware already has a full clear-and-draw pass; partial invalidation would add complexity without a clear payoff for this first stage.
  - Alternative considered: per-widget redraw. Rejected because it would require region tracking and more renderer state.

- Derive periodic redraw cadence from the minimum effective interval across dynamic text widgets.
  - Rationale: one timer is enough for the whole page and matches the existing redraw model.
  - Alternative considered: independent timers per widget. Rejected because it increases runtime overhead and does not improve the current rendering strategy.

## Risks / Trade-offs

- [Risk] A page with one fast dynamic widget will refresh all widgets at that cadence.
  - [Mitigation] Keep intervals bounded and rely on the existing whole-layout redraw model until a measured need for partial redraw appears.

- [Risk] Placeholder resolution can differ between the catalog preview and runtime state if the source changes quickly.
  - [Mitigation] Treat the catalog as a best-effort preview surface and keep the runtime renderer authoritative.

- [Risk] Soft-failing unresolved placeholders can hide missing source problems from users.
  - [Mitigation] Surface availability state in the designer so the user can see when a selected placeholder is currently unavailable.

- [Risk] New namespaces may be added later and require coordinated updates.
  - [Mitigation] Keep namespace parsing centralized in the metric catalog/resolver layer instead of scattering string checks through widgets.
