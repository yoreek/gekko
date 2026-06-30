## 1. Firmware config and runtime

- [x] 1.1 Add an explicit display orientation or rotation field to the display device config model and keep it versioned for migration.
- [x] 1.2 Apply the persisted rotation during OLED and ST7735 hardware initialization so runtime rendering matches the mounted panel.
- [x] 1.3 Add safe migration defaults for legacy configs that do not yet contain orientation data.

## 2. Portal designer and previews

- [x] 2.1 Update the display designer and device form UIs to expose logical orientation groups instead of raw upside-down rotation choices.
- [x] 2.2 Make layout preview and canvas sizing orientation-aware so widget bounds use the effective width and height.
- [x] 2.3 Ensure ST7735 and SSD1306 previews open with their correct family-specific default orientations.

## 3. Tests and regression coverage

- [x] 3.1 Add unit coverage for config migration and raw rotation persistence.
- [x] 3.2 Add UI or component coverage for orientation-aware preview geometry and layout bounds.
- [x] 3.3 Verify both display families keep their expected default orientation after loading existing devices.
