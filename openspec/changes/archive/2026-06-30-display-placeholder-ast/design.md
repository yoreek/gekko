## Context

Display text widgets currently keep placeholder syntax in raw text and resolve it during render-time evaluation. That is acceptable for a small number of widgets, but it puts parsing work on the hot path and leaves validation split between the designer and the runtime. The current runtime also treats malformed placeholders as a whole-widget concern, which makes the draw loop do more work than necessary and makes error handling less precise than it should be.

This change affects both the portal designer and the firmware render pipeline for the shared `ssd1306` and `st7735` display stack. The existing layout contract already carries stable device IDs and metric IDs, so the design can build a compiled placeholder form without changing persisted layout ownership.

## Goals / Non-Goals

**Goals:**
- Parse text-widget placeholder strings once into a structured intermediate representation.
- Validate placeholder syntax, referenced device existence, and referenced metric existence during designer validation and/or layout compilation.
- Support any number of placeholders in a single text widget.
- Keep render-time work bounded by reusing compiled placeholder segments instead of reparsing raw text.
- Preserve the raw text value as the persisted source of truth.
- Keep invalid or missing placeholders soft-failed per placeholder, not as a fatal layout-wide error.
- Track placeholder source devices as registry dependencies so referenced devices cannot be deleted while a display layout depends on them.

**Non-Goals:**
- Reworking the device registry or adding new placeholder namespaces.
- Changing the JSON layout shape at the API boundary unless required for validation metadata.
- Adding a visual expression editor beyond the existing text field and picker flow.
- Migrating old persisted display layouts that were saved before this feature existed.

## Decisions

### 1. Use a runtime-only compiled placeholder representation alongside the stored raw text
The raw widget `text` remains the persisted source of truth, but the display runtime should compile it into a small structured form containing literal spans and placeholder references. The compiled form belongs to the runtime object and is never written to NVS because it may contain pointers to runtime objects in memory.

Rationale: this keeps round-trip behavior stable while moving parsing cost out of the draw loop. It also allows validation and rendering to share the same parser output.

Alternatives considered:
- Parse on every render: simplest, but too expensive for frequent redraws.
- Persist the compiled form in JSON: faster to load, but adds schema churn and migration cost for a representation that is derivable from text.

### 2. Represent placeholders as typed references, not just strings
The compiled form should keep namespace plus resolved reference data for device and metric lookup, such as namespace enum, source device ID, metric ID, and optional filter metadata. Runtime pointers are allowed only in the transient AST. Metric resolution should feed a typed `MetricValue` contract with a bounded set of payload kinds, rather than collapsing everything to string too early. A practical value model is `Null`, `Bool`, `Int`, `Float`, and `String`.

Rationale: typed references make validation explicit and let the renderer skip repeated namespace/source/metric parsing. A typed value contract preserves numeric and boolean meaning until the final formatting step, which makes filters useful for more than string casing.

Alternatives considered:
- Store only normalized strings: less coupling, but still requires parsing and lookup work during render.
- Persist direct pointers or compiled handles: impossible because the data is memory-resident and not valid across reboot.

### 3. Validate at save time, degrade softly at render time
Save-time validation should reject malformed syntax and report missing device or metric references to the designer/API layer. Runtime rendering should still tolerate unresolved segments by substituting empty strings for those segments and continuing with the rest of the text.

Rationale: users need immediate feedback while editing, but the display should not fail a full refresh because one placeholder is stale.

Alternatives considered:
- Reject the whole widget on any missing reference: simpler validation, but too brittle for operational dashboards.
- Ignore validation entirely: keeps the draw path simple now, but pushes all errors into runtime and makes the designer less useful.

### 4. Keep compilation as a reusable service, not display-backend specific logic
The parser/compiled representation should live in the shared display text domain, then be consumed by SSD1306 and ST7735 renderers. Filters belong to this shared text domain as a bounded formatting layer that runs after typed metric resolution and before final draw-time text layout.

Rationale: placeholder syntax is a text-domain concern, not a panel-specific detail. Shared parsing avoids divergence between backends.

### 5. Cache compilation close to layout update boundaries
Layout mutation boundaries should invalidate the runtime text cache. The renderer may then lazily build or refresh the compiled AST on the next render pass, using the already accepted raw layout text and dependency-safe references.

Rationale: this preserves a low-cost render loop while still allowing the layout text to remain editable.

### 6. Extract metric source dependencies from accepted layouts
The firmware should parse structured placeholders during layout create/update validation, collect unique `dev.<id>` source device IDs, validate that each source device exists, and validate that the source device type supports the requested metric. Accepted display layouts should update the display registry dependencies to include its hardware bus dependency plus those metric source devices.

Rationale: this lets the existing registry deletion protection reject deletion of devices that a display layout still references. It also keeps the dependency graph persistent across reboot without persisting the runtime AST. The persisted `metric_source` link is the source of truth; transient runtime pointers are only an execution cache.

### 7. Treat dependencies as a list of links, not unique role slots
The registry should treat `deps` as a list of dependency links where `role` describes the relationship type but is not a unique key. Duplicate roles are allowed at the base registry level. Device-specific validation remains responsible for cardinality rules, such as requiring exactly one hardware bus dependency for a display.

Display devices should use a new `DeviceDependencyRole::MetricSource` for placeholder source devices. A single display may have multiple `MetricSource` dependency links, one per unique source device referenced by the accepted layout.

Rationale: placeholder source devices are content dependencies, not one fixed bus dependency. Encoding them as `MetricSource1`, `MetricSource2`, and similar numbered roles would leak implementation detail into the public dependency contract.

Runtime dependency pointers should be stored and synchronized by dependency index. Before replacing dependency links, the runtime should detach every old dependency runtime, clear all dependency pointers, then attach every new dependency runtime by matching each list entry's `deviceId`. The existing `dependencyRuntime(role)` helper may remain as a first-matching-role compatibility helper for existing single-role device code, but new code should be able to inspect the full dependency list or access dependency runtimes by index.

The runtime AST should resolve metric source runtimes through the render resolver/context by `DeviceId`, not through `dependencyRuntime(MetricSource)`.

### 8. Expand display text capacity to 128 bytes
`kDisplayLayoutTextCapacity` should increase from 32 bytes to 128 bytes so realistic multi-placeholder text fits in one widget while keeping display sidecar growth bounded. This changes the binary display layout record shape and must follow the device config/versioning and binary compatibility path.

### 9. Do not migrate old display layouts
No migration is required for old display layouts without metric-source dependencies because this feature is not in production. Old layouts are not a compatibility target for this change.

## Risks / Trade-offs

- [Risk] Cached compiled data can become stale if the raw text changes without invalidation → Mitigation: invalidate runtime AST caches on layout and dependency mutation paths, then rebuild lazily during rendering.
- [Risk] Direct runtime references can become invalid if dependency deletion or relinking bypasses display dependency tracking → Mitigation: parse placeholders at save time, persist `MetricSource` dependency links, and rely on registry deletion protection.
- [Risk] Validation may reject content that current users have been relying on informally → Mitigation: keep soft-fail render behavior and make validation messages explicit in the designer.
- [Risk] Adding a parser and cache may introduce duplication between frontend and backend placeholder logic → Mitigation: keep the placeholder grammar and normalized output shape documented and covered by tests on both sides.
- [Risk] Increasing display text capacity grows binary layout records and sidecar payloads → Mitigation: bump/check the layout record compatibility path and verify maximum layout payload size remains within display sidecar bounds.

## Migration Plan

1. Introduce the shared placeholder parser and compiled segment model.
2. Remove registry-wide dependency role uniqueness, add `MetricSource`, synchronize runtime dependency pointers by list index, and raise display dependency capacity to 16.
3. Wire designer/API validation to the parser so invalid syntax and missing references are surfaced before save.
4. Extract metric source dependencies from accepted layouts and persist them with the display registry record.
5. Update runtime evaluation to lazily build and use the runtime AST, then fall back to empty substitutions for unresolved placeholders.
6. Add or update tests for multi-placeholder parsing, validation, dependency extraction, deletion protection, and render fallbacks.
7. Document the new placeholder model and update any relevant display docs.

Rollback is straightforward because raw text remains the persisted contract. If the compiled form causes issues, the renderer can temporarily fall back to the legacy raw-text parser while keeping the stored data unchanged.

## Open Questions

- None currently. The implementation path is to persist raw text plus registry dependency links, keep AST state runtime-only, validate strictly at save time, and render unresolved runtime values as empty text.
