## 1. OpenSpec Artifacts

- [x] 1.1 Add delta specs for `display-metric-placeholders`, `oled-display-layout-designer`, and `oled-display`.
- [x] 1.2 Add the new `display-text-placeholder-ast` capability spec.
- [x] 1.3 Validate the OpenSpec change status before implementation begins.

## 2. Firmware Parser And Compiled Model

- [ ] 2.1 Add shared display text segment, compiled widget, compile status, and compile result types.
- [ ] 2.2 Implement placeholder parsing for literal segments and `dev`, `system`, and `wifi` placeholder segments.
- [ ] 2.3 Implement validation context checks for namespace rules, positive device IDs, known devices, known metric keys, and output capacity.
- [ ] 2.4 Keep legacy `{value}` and `bindingKind=Metric` compatibility separate from the structured `{{...}}` parser path.

## 3. Firmware Runtime Integration

- [ ] 3.1 Store compiled text data as transient runtime/cache state, not in JSON or binary persisted layout payloads.
- [ ] 3.2 Rebuild compiled text data from raw layout text in `loadPersistedState`, `applyPersistedStateUpdate`, and `setLayout`.
- [ ] 3.3 Update `DisplayLayoutRenderSession` to evaluate compiled text segments during rendering instead of reparsing raw widget text.
- [ ] 3.4 Preserve tolerant runtime behavior where unresolved or stale placeholder segments render as empty text and the remaining text still draws.
- [ ] 3.5 Preserve dynamic-widget refresh cadence using compiled placeholder state.

## 4. Backend Layout Validation

- [ ] 4.1 Validate structured text placeholders before accepting SSD1306 create layout payloads.
- [ ] 4.2 Validate structured text placeholders before accepting SSD1306 update layout payloads.
- [ ] 4.3 Validate structured text placeholders before accepting ST7735 create layout payloads.
- [ ] 4.4 Validate structured text placeholders before accepting ST7735 update layout payloads.
- [ ] 4.5 Return `400` for malformed syntax, unknown namespaces, unknown source devices, and unknown metric keys.
- [ ] 4.6 Verify JSON and binary layout round trips still preserve raw widget `text`.

## 5. Frontend Validation

- [ ] 5.1 Align SPA placeholder parsing with backend grammar for multiple placeholders per text widget.
- [ ] 5.2 Report valid, invalid, unavailable or missing, and static placeholder counts in validation state.
- [ ] 5.3 Allow picker insertion to add more than one placeholder into existing text.
- [ ] 5.4 Block designer save when frontend validation finds invalid placeholders.
- [ ] 5.5 Confirm long placeholder text respects the display text capacity or update the capacity through the documented config-versioning path.

## 6. Documentation

- [ ] 6.1 Document that any number of placeholders is allowed within text capacity.
- [ ] 6.2 Document strict REST save validation for invalid syntax, unknown devices, and unknown metrics.
- [ ] 6.3 Document tolerant runtime substitution where unresolved placeholders render as empty text.
- [ ] 6.4 Document that compiled placeholder data is transient and raw widget text remains the persisted source of truth.

## 7. Verification

- [ ] 7.1 Add firmware parser tests for static text, one placeholder, multiple placeholders, malformed braces, unknown namespace, missing or zero device ID, unknown device, and unknown metric key.
- [ ] 7.2 Add firmware rendering tests for multiple placeholder resolution, missing metric empty substitution, stale compiled reference tolerance, and refresh cadence.
- [ ] 7.3 Add REST/layout tests for SSD1306 and ST7735 create/update acceptance and rejection paths.
- [ ] 7.4 Add frontend tests for independent multi-placeholder validation, repeated picker insertion, save blocking, and long placeholder capacity handling.
- [ ] 7.5 Run `scripts/test.sh` after implementation is complete.
