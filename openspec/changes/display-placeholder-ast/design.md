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

**Non-Goals:**
- Reworking the device registry or adding new placeholder namespaces.
- Treating deleted devices as a solved dependency problem in this change.
- Changing the JSON layout shape at the API boundary unless required for validation metadata.
- Adding a visual expression editor beyond the existing text field and picker flow.

## Decisions

### 1. Use a compiled placeholder representation alongside the stored raw text
The raw widget `text` remains the persisted source of truth, but the runtime should compile it into a small structured form containing literal spans and placeholder references.

Rationale: this keeps round-trip behavior stable while moving parsing cost out of the draw loop. It also allows validation and rendering to share the same parser output.

Alternatives considered:
- Parse on every render: simplest, but too expensive for frequent redraws.
- Persist the compiled form in JSON: faster to load, but adds schema churn and migration cost for a representation that is derivable from text.

### 2. Represent placeholders as typed references, not just strings
The compiled form should keep namespace plus resolved reference data for device and metric lookup, such as namespace enum, device ID or direct device reference, and metric ID or resolver handle.

Rationale: typed references make validation explicit and let the renderer skip repeated namespace/source/metric parsing. A later pass can decide whether to hold direct pointers, IDs, or lightweight resolver handles depending on lifetime constraints.

Alternatives considered:
- Store only normalized strings: less coupling, but still requires parsing and lookup work during render.
- Store direct pointers only: fast, but risky while device lifetime and deletion handling are still evolving.

### 3. Validate at compile time, degrade softly at render time
Validation should reject malformed syntax and report missing device or metric references to the designer/API layer. Runtime rendering should still tolerate unresolved segments by substituting empty strings for those segments and continuing with the rest of the text.

Rationale: users need immediate feedback while editing, but the display should not fail a full refresh because one placeholder is stale.

Alternatives considered:
- Reject the whole widget on any missing reference: simpler validation, but too brittle for operational dashboards.
- Ignore validation entirely: keeps the draw path simple now, but pushes all errors into runtime and makes the designer less useful.

### 4. Keep compilation as a reusable service, not display-backend specific logic
The parser/compiled representation should live in the shared display text domain, then be consumed by SSD1306 and ST7735 renderers.

Rationale: placeholder syntax is a text-domain concern, not a panel-specific detail. Shared parsing avoids divergence between backends.

### 5. Cache compilation close to layout update boundaries
The compiled structure should be produced when the widget text changes, the layout is loaded, or the designer saves a new draft. The renderer should receive the compiled object directly or via a cheap lazy cache lookup.

Rationale: this preserves a low-cost render loop while still allowing the layout text to remain editable.

## Risks / Trade-offs

- [Risk] Cached compiled data can become stale if the raw text changes without invalidation → Mitigation: tie the cache to widget text versioning or regenerate on layout mutation paths.
- [Risk] Direct device references can complicate deletion or rebind behavior later → Mitigation: start with ID-based or handle-based references that can be revalidated.
- [Risk] Validation may reject content that current users have been relying on informally → Mitigation: keep soft-fail render behavior and make validation messages explicit in the designer.
- [Risk] Adding a parser and cache may introduce duplication between frontend and backend placeholder logic → Mitigation: keep the placeholder grammar and normalized output shape documented and covered by tests on both sides.

## Migration Plan

1. Introduce the shared placeholder parser and compiled segment model.
2. Wire designer validation to the parser so invalid syntax and missing references are surfaced before save.
3. Update runtime evaluation to use the compiled form and fall back to empty substitutions for unresolved placeholders.
4. Add or update tests for multi-placeholder parsing, validation, and render fallbacks.
5. Document the new placeholder model and update any relevant display docs.

Rollback is straightforward because raw text remains the persisted contract. If the compiled form causes issues, the renderer can temporarily fall back to the legacy raw-text parser while keeping the stored data unchanged.

## Open Questions

- Should the compiled placeholder form store direct pointers, stable IDs, or lightweight resolver handles for device and metric references?
- Should validation fail the save action on missing devices/metrics, or only surface warnings while allowing drafts to persist?
- Should the parser live in firmware-only code first, or should the portal share the same normalized grammar and output shape immediately?
