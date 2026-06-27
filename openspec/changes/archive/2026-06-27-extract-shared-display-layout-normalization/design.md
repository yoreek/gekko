## Context

The portal already has shared display model primitives in `portal-spa/src/models/devices/display/layout.ts` and shared canvas math in `portal-spa/src/components/devices/display/core/display-layout-math.ts`. The normalization implementation is still anchored in `portal-spa/src/models/devices/ssd1306/layout.ts`, including generic page/widget parsing, widget defaults, bounds, JSON encoding, and layout comparison.

ST7735 is not a complete display implementation yet. This change does not try to complete it. The current goal is to remove the avoidable SSD1306 ownership from common display layout code so later controller work can reuse the same normalization path.

## Goals / Non-Goals

**Goals:**
- Create a shared display layout profile type for controller-specific limits and defaults.
- Create a shared display layout normalizer/encoder/change-comparison module.
- Keep the existing SSD1306 exports and behavior stable through thin wrappers.
- Make the ST7735 layout model use the shared normalizer without adding ST7735 designer UI.
- Keep model code independent from Vue component paths where possible.
- Add focused unit tests proving shared normalization and wrapper behavior.

**Non-Goals:**
- No firmware layout, binary codec, registry type, or REST API changes.
- No ST7735 designer, form/detail, or rendering runtime implementation.
- No full rewrite of `Ssd1306DesignerDialog.vue` or designer components.
- No behavior change to dashboard widgets.
- No new dependencies.

## Decisions

### Add a profile object instead of hardcoding controller branches

Add `portal-spa/src/models/devices/display/profile.ts` with a `DisplayLayoutProfile` contract. The profile owns display-controller defaults such as bitmap format, bitmap default dimensions, max bitmap bytes, text/page limits, default text, and allowed widget types.

Rationale: a profile keeps the shared normalizer generic without adding `if (controller === ...)` branches. SSD1306 can define a complete profile now. ST7735 can define a minimal profile now and grow later.

Alternative considered: move SSD1306 code into a common file unchanged. Rejected because the common layer would still encode SSD1306 assumptions in names and constants.

### Add a shared normalizer module with wrappers

Add `portal-spa/src/models/devices/display/layout-normalizer.ts` for:
- default layout creation
- default widget creation
- widget normalization
- layout normalization
- layout encoding
- layout change comparison

Keep `portal-spa/src/models/devices/ssd1306/layout.ts` exporting the existing function and type names by delegating to the shared normalizer with the SSD1306 profile.

Rationale: this gives a small, safe refactor step. Existing imports in designer components and tests remain valid while the duplicated logic moves to the correct domain.

Alternative considered: update all consumers to import from the new shared module immediately. Rejected because it creates a wider diff without improving behavior.

### Do not generalize designer components in this change

The designer components contain common editor behavior, but extracting them now would mix model normalization with UI refactoring. This change stops at the model layer.

Rationale: the first useful boundary is stable layout normalization. Once this is in place, designer extraction can be a separate change with smaller risk.

Alternative considered: rename all `Ssd1306Designer*` components to `DisplayDesigner*`. Rejected because ST7735 UI is not ready and the user asked for the current concrete step.

### Keep ST7735 as a model-only consumer

Update `portal-spa/src/models/devices/st7735/layout.ts` so it uses the shared normalizer and preserves pages/widgets instead of only carrying `colorMode`.

Rationale: this validates that the shared normalizer is not just SSD1306 code moved into another folder, without claiming ST7735 UI completeness.

Alternative considered: leave ST7735 untouched. Rejected because then the change would not prove the common layer can serve a second display profile.

## Risks / Trade-offs

- [Risk] Moving normalization may accidentally change SSD1306 encoded layout shape.
  - Mitigation: keep SSD1306 wrapper functions and add tests that compare default widgets, page limits, bitmap payload behavior, and layout changed comparison.
- [Risk] ST7735 gets stronger layout normalization before its UI is implemented.
  - Mitigation: limit ST7735 changes to model normalization and tests; do not register new UI or command behavior in this change.
- [Risk] The shared profile may be too small for future display controllers.
  - Mitigation: keep the profile focused on values needed by existing normalization only; expand later when a concrete controller needs more fields.
- [Risk] Text autosize currently depends on SSD1306 classic-font behavior.
  - Mitigation: keep existing SSD1306 autosize behavior behind the SSD1306 wrapper/profile path and do not introduce a generic font abstraction in this change.
