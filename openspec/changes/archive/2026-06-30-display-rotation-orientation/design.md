## Context

The display stack currently mixes three concerns:

- hardware rotation used by the Adafruit drivers at runtime
- layout geometry used by the portal designer and preview
- physical mounting defaults that differ between display families

Today ST7735 hardcodes `setRotation(0)`, while SSD1306 does not expose an explicit orientation contract in the same way. The portal designer also treats the canvas as if orientation were implicit, which makes preview and saved layouts depend on hidden assumptions instead of a shared model.

The change needs to work across both OLED and TFT display families, preserve existing layouts, and avoid forcing users to think in upside-down 90-degree rotations while still keeping the full hardware rotation range available to firmware.

## Goals / Non-Goals

**Goals:**

- Make display orientation explicit in the device contract.
- Let the designer work with logical orientation groups instead of raw hardware rotation values.
- Keep firmware capable of using raw rotation values `0..3`.
- Make preview, widget bounds, and device forms follow the effective display orientation.
- Support family-specific defaults so ST7735 and SSD1306 can open in different mounted orientations.
- Preserve backward compatibility for existing stored configs and layouts.

**Non-Goals:**

- Adding arbitrary angle rotation beyond the four hardware rotations.
- Changing widget rendering rules beyond the coordinate-space impact of display orientation.
- Introducing a separate runtime rotation mode per widget or per page.
- Reworking the full display rendering pipeline beyond orientation-aware sizing and initialization.

## Decisions

### 1. Persist raw hardware rotation in firmware config

The firmware will keep a raw `rotation` value in the device config as the source of truth for runtime initialization.

Reasoning: Adafruit drivers already understand `0..3`, and the firmware needs that exact value to initialize the panel correctly. A logical-only config would make runtime mapping ambiguous and would hide the hardware contract.

Alternative considered: store only `portrait` / `landscape` in config. Rejected because the hardware still needs an exact rotation and the project needs to preserve the ability to distinguish physical mounting variants.

### 2. Expose logical orientation groups in the designer

The portal designer will present orientation as a human-facing logical model rather than a raw rotation selector. The designer can treat `0/2` as one group and `1/3` as the other group, so the user does not have to design on an upside-down canvas.

Reasoning: users think in terms of the mounted panel direction, not in terms of Adafruit rotation constants. A grouped selector keeps the editor understandable while still allowing the underlying runtime value to be chosen deterministically.

Alternative considered: expose the raw `0..3` rotation values directly. Rejected because it leaks hardware detail into the UI and makes the layout workflow error-prone.

### 3. Make preview and bounds orientation-aware

Preview components and layout canvas sizing will use the effective orientation when calculating width, height, widget bounds, and rendered preview frames.

Reasoning: if the preview uses one coordinate space and firmware uses another, the user will continue to design against the wrong geometry. Orientation must affect the same logical canvas that editing uses.

Alternative considered: rotate the rendered preview only with CSS or a visual transform. Rejected because that hides coordinate-space changes and would not keep hit testing, bounds, and widget geometry aligned with the actual layout.

The persisted `width` and `height` remain the physical panel dimensions. Internally, the implementation may treat these as `physWidth` and `physHeight` to make the distinction explicit. The editor and runtime derive `effectiveWidth` and `effectiveHeight` from the stored physical dimensions plus orientation, and all bounds checks use the derived values. Widget coordinates remain stable in the layout's logical coordinate space; changing orientation does not rewrite widget `x`, `y`, `width`, or `height`.

### 4. Allow display-family defaults to differ

ST7735 and SSD1306 will each have their own default orientation mapping so the designer and runtime open in the orientation that matches the current physical mounting assumptions.

Reasoning: the current devices are mounted differently, so one global default would keep one family wrong. The default needs to be family-specific, while still allowing the user to override it.

Alternative considered: force a single default orientation across all display families. Rejected because it would keep at least one family visually inverted or rotated relative to its physical installation.

### 5. Version the config change and migrate safely

Device config schema changes will carry the new orientation field with a defined default and migration path.

Reasoning: existing devices should keep rendering after upgrade without requiring manual edits. The migration must preserve current behavior for legacy configs and then let users adjust the new orientation explicitly if needed.

Alternative considered: treat orientation as a transient UI-only option. Rejected because the firmware runtime needs the same contract to render the panel correctly after reboot.

## Risks / Trade-offs

- [Risk] A wrong default rotation mapping could flip one display family after upgrade.
  → Mitigate by using family-specific defaults, keeping migration tests, and verifying the physical mounting assumptions for both display types before release.

- [Risk] Designer and firmware could drift if only one side is updated.
  → Mitigate by making orientation part of the persisted config contract and by covering preview, form, and runtime paths in tests.

- [Risk] Backward compatibility may preserve a legacy orientation that is technically valid but visually unexpected on some devices.
  → Mitigate by keeping the legacy raw rotation default during migration unless the device family has an explicit new default.

- [Risk] A logical orientation abstraction may still need an advanced override for troubleshooting.
  → Mitigate by keeping raw rotation in the firmware contract even if the common UI only exposes the grouped view.

## Migration Plan

1. Add the orientation/rotation field to the display device config schema with a stable default.
2. Migrate existing configs so legacy records continue to load, keeping stored physical `width` and `height` intact and applying family-specific defaults where needed.
3. Update the portal designer and device previews to derive effective canvas dimensions from physical size plus orientation, without rewriting widget coordinates.
4. Update firmware initialization to apply the stored rotation for both SSD1306 and ST7735 and to use derived effective dimensions when validating layout bounds.
5. Add regression tests for config migration, preview geometry, and runtime initialization.
6. If a mismatch is found on real hardware, adjust the family-specific default mapping rather than changing the layout contract.

## Open Questions

- What exact raw rotation should be the default for each display family in the current physical mounting?
- Should the UI expose only the grouped orientation model, or should an advanced raw rotation override remain visible?
- For already-saved devices, should migration keep the previous raw rotation as-is or normalize by display family when the new field is introduced?
