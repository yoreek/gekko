## 1. OpenSpec Artifacts

- [x] 1.1 Add delta specs for `display-metric-placeholders`, `oled-display-layout-designer`, and `oled-display`.
- [x] 1.2 Add the new `display-text-placeholder-ast` capability spec.
- [x] 1.3 Validate the OpenSpec change status before implementation begins.

## 2. Firmware Parser And Compiled Model

- [x] 2.1 Add shared display text segment, compiled widget, compile status, and compile result types.
- [x] 2.2 Implement placeholder parsing for literal segments and `dev`, `system`, and `wifi` placeholder segments.
- [ ] 2.3 Implement validation context checks for namespace rules, positive device IDs, known devices, device-type-supported metric keys, and output capacity.
- [x] 2.4 Keep legacy `{value}` and `bindingKind=Metric` compatibility separate from the structured `{{...}}` parser path.
- [x] 2.5 Keep the compiled AST runtime-only and allow it to hold memory references that are never persisted.

## 3. Dependency Graph Integration

- [x] 3.1 Increase `kMaxDeviceDependencies` to 16.
- [x] 3.2 Add `DeviceDependencyRole::MetricSource` and expose it as `metric_source` in dependency role parsing/serialization.
- [x] 3.3 Remove registry-wide dependency role uniqueness so repeated roles are accepted as separate dependency list entries.
- [x] 3.4 Add dependency runtime access by list index and synchronize runtime dependency pointers by replacing the full dependency list.
- [x] 3.5 Keep `dependencyRuntime(role)` as a first-matching-role compatibility helper for existing single-role device code.
- [x] 3.6 Enforce single-role cardinality in device-specific config or adapter validation where a device requires exactly one dependency for a role.
- [x] 3.7 Verify registry deletion protection reports displays as dependents for devices referenced by `metric_source` links.

## 4. Firmware Runtime Integration

- [x] 4.1 Store compiled text data as transient runtime/cache state, not in JSON or binary persisted layout payloads.
- [x] 4.2 Invalidate compiled text data in `loadPersistedState`, `applyPersistedStateUpdate`, and `setLayout`.
- [x] 4.3 Lazily rebuild compiled text AST during rendering when the cache is missing or invalid.
- [x] 4.4 Update `DisplayLayoutRenderSession` to evaluate compiled text segments during rendering instead of reparsing raw widget text.
- [x] 4.5 Preserve tolerant runtime behavior where unresolved or stale placeholder segments render as empty text and the remaining text still draws.
- [x] 4.6 Preserve dynamic-widget refresh cadence using compiled placeholder state.

## 5. Backend Layout Validation

- [x] 5.1 Validate structured text placeholders before accepting SSD1306 create layout payloads.
- [x] 5.2 Validate structured text placeholders before accepting SSD1306 update layout payloads.
- [x] 5.3 Validate structured text placeholders before accepting ST7735 create layout payloads.
- [x] 5.4 Validate structured text placeholders before accepting ST7735 update layout payloads.
- [x] 5.5 Extract unique metric source device IDs from accepted layouts and persist them as `metric_source` dependencies with the hardware bus dependency.
- [x] 5.6 Return `400` for malformed syntax, unknown namespaces, unknown source devices, and unknown or unsupported metric keys.
- [x] 5.7 Verify JSON and binary layout round trips still preserve raw widget `text`.

## 6. Frontend Validation

- [x] 6.1 Align SPA placeholder parsing with backend grammar for multiple placeholders per text widget.
- [x] 6.2 Report valid, invalid, unavailable or missing, and static placeholder counts in validation state.
- [ ] 6.3 Allow picker insertion to add more than one placeholder into existing text.
- [ ] 6.4 Block designer save when frontend validation finds invalid placeholders.
- [ ] 6.5 Increase display text capacity handling to 128 bytes.

## 7. Storage And Documentation

- [x] 7.1 Increase `kDisplayLayoutTextCapacity` to 128 bytes and update display layout binary record compatibility as needed.
- [ ] 7.2 Verify maximum supported display layout payload still fits supported sidecar bounds.
- [x] 7.3 Document that any number of placeholders is allowed within the 128-byte text capacity.
- [x] 7.4 Document strict REST save validation for invalid syntax, unknown devices, and unknown or unsupported metrics.
- [x] 7.5 Document tolerant runtime substitution where unresolved placeholders render as empty text.
- [x] 7.6 Document that compiled placeholder data is transient and raw widget text plus registry dependencies remain the persisted source of truth.
- [x] 7.7 Document that old layouts are not migrated because this feature is not in production.

## 8. Verification

- [ ] 8.1 Add firmware parser tests for static text, one placeholder, multiple placeholders, malformed braces, unknown namespace, missing or zero device ID, unknown device, unknown metric key, and unsupported metric for source device type.
- [ ] 8.2 Add registry tests for repeated-role dependency lists, index-based runtime dependency synchronization, device-specific cardinality validation, and deletion protection.
- [ ] 8.3 Add firmware rendering tests for multiple placeholder resolution, missing metric empty substitution, stale compiled reference tolerance, lazy AST rebuild, and refresh cadence.
- [ ] 8.4 Add REST/layout tests for SSD1306 and ST7735 create/update acceptance, dependency extraction, and rejection paths.
- [ ] 8.5 Add frontend tests for independent multi-placeholder validation, repeated picker insertion, save blocking, and 128-byte capacity handling.
- [x] 8.6 Run `scripts/test.sh` after implementation is complete.
