## Context

The portal currently treats metric placeholders as a display-specific concern. Parsing, validation, filter application, and preview resolution are concentrated in `models/metrics/placeholders.ts`, while the display designer directly consumes metric catalog entries for validation and preview behavior.

That coupling creates two problems:

1. The display designer preview shows raw placeholder source text instead of resolved sample values, so the editor does not represent real rendered output.
2. Placeholder parsing and filtering are not isolated enough to be tested as a generic text templating concern.

This change is UI-scoped. The first consumer is the display designer in `portal-spa`, and no firmware or backend API changes are required.

## Goals / Non-Goals

**Goals:**
- Extract a reusable template engine that parses placeholder tokens, extracts metadata, renders from a `name -> value` resolver, and applies filters.
- Keep the engine independent from metrics and display widgets.
- Use the engine in the display designer UI so preview text and sizing use resolved sample values when available.
- Preserve raw widget text as the source of truth in saved configs and runtime payloads.
- Keep the implementation covered by small, focused unit tests.

**Non-Goals:**
- Do not change firmware placeholder evaluation or display runtime rendering behavior in this change.
- Do not change the persisted display layout schema or widget config shape.
- Do not add a new backend endpoint.
- Do not make the template engine a general-purpose server-side rendering system.

## Decisions

### 1. Create a generic template engine in the portal SPA
Implement a standalone templating module that understands `{{name}}` tokens plus a bounded filter pipeline. The module should operate on plain text and a resolver function or flat object map, not on display widgets or metrics.

Rationale:
- Keeps the parsing/rendering logic reusable.
- Allows direct unit coverage without display-specific setup.
- Makes the engine usable by other UI surfaces later without reworking the parser again.

Alternatives considered:
- Keep the current metric placeholder helper and add more methods there. Rejected because it preserves the current coupling.
- Add a third-party templating library. Rejected because the required feature set is small and the repo is bundle-sensitive.

### 2. Keep metrics as an adapter layer
Keep metric-specific catalog lookup in a thin adapter that converts metric descriptors into the resolver data the generic engine expects.

Rationale:
- Metrics still own the source labels, preview values, availability state, and metric IDs.
- The generic engine should not know about namespaces, device IDs, or metric catalog transport.

Alternatives considered:
- Push metric logic into the engine. Rejected because it would reintroduce domain coupling.

### 3. Use resolved sample text only in the designer UI
The display designer preview and fit/auto-size calculations should use resolved sample values when the metric catalog has them. The raw template remains what the user edits and what gets saved.

Rationale:
- This solves the immediate UX problem without altering stored configuration.
- The designer remains honest when sample values are unavailable by falling back to the raw token.

Alternatives considered:
- Render raw template text everywhere. Rejected because that is the current problem.
- Replace missing values with blank text in the designer UI. Rejected because it hides unresolved placeholders and makes debugging harder.

### 4. Preserve the existing validation contract
Keep malformed placeholders and unsupported filters invalid, but preserve current save-time behavior where the raw widget text remains the canonical value.

Rationale:
- This avoids migration work.
- It keeps the change additive for the portal UI.

## Risks / Trade-offs

- [Risk] The new engine may diverge from the firmware placeholder parser over time. → Mitigation: keep the template syntax and filter set aligned with the existing placeholder spec and add regression tests for shared examples.
- [Risk] UI preview may still differ from runtime if sample values are stale or unavailable. → Mitigation: make the fallback explicit and keep the raw template visible in the editor field.
- [Risk] The refactor may touch several display components at once. → Mitigation: keep the engine pure, move metric adaptation into a small wrapper, and update the designer with computed preview text instead of duplicating logic in each widget component.
- [Risk] Over-generalizing the API could make the engine harder to use. → Mitigation: keep the first version small: parse, extract, render, and validate only.

## Migration Plan

1. Add the generic template engine module alongside the current metric placeholder helper.
2. Re-implement metric placeholder parsing/rendering as a wrapper around the generic engine.
3. Switch the display designer inspector to compute resolved preview text from the metric catalog sample values.
4. Update fit/auto-size calculations to measure the resolved preview string.
5. Keep the existing raw widget text and validation flow unchanged.
6. Add unit tests for the template engine and update designer tests to verify resolved preview behavior.

Rollback:
- If the new engine causes regressions, revert the designer to the existing metric placeholder helper while keeping the new module isolated.

## Open Questions

- Should the initial engine API accept only a flat `Record<string, string>` resolver, or should it also accept a callback resolver for future dynamic use cases?
- Should unresolved placeholders remain raw in the designer preview, or should specific surfaces be allowed to opt into blank substitution later?
- Should the generic engine live under `src/models/template/` or `src/models/text-template/` for long-term naming clarity?
