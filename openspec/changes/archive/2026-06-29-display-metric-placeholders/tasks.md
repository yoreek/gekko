## 1. Metric catalog

- [ ] 1.1 Expose a structured metric placeholder catalog with `dev`, `system`, and `wifi` namespaces.
- [ ] 1.2 Include stable source IDs, source labels, metric keys, and availability state in catalog responses.

## 2. Designer workflow

- [ ] 2.1 Add a placeholder picker that lets the user choose namespace, source device, and metric.
- [ ] 2.2 Insert normalized placeholder strings into text widgets from the picker.
- [ ] 2.3 Show validation and availability state without blocking edits for soft-missing metrics.
- [ ] 2.4 Reveal a bounded refresh interval control for text widgets that contain dynamic placeholders.

## 3. Firmware rendering

- [ ] 3.1 Resolve structured placeholders during display text evaluation.
- [ ] 3.2 Keep unresolved placeholders soft-failing so layouts stay renderable when sources are missing.
- [ ] 3.3 Reuse the existing full clear-and-draw render pass for refresh-driven updates.
- [ ] 3.4 Compute periodic redraw cadence from the minimum effective interval across dynamic widgets.

## 4. Verification

- [ ] 4.1 Add native tests for catalog generation, placeholder resolution, and missing-source behavior.
- [ ] 4.2 Add native tests for layout redraw ordering and minimum refresh interval selection.
- [ ] 4.3 Add SPA unit coverage for placeholder insertion, validation, and refresh interval visibility.
