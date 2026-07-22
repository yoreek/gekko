# Translation plan

This file tracks the translation workflow for `docs-site/`.

## Rules

- Translate one locale at a time.
- Work one file at a time, or in small batches only when explicitly allowed.
- Before marking a file done, verify:
  - frontmatter still matches the source structure;
  - headings are present and in the same order;
  - links are not checked or fixed yet;
  - image and diagram paths are correct for the localized nesting level;
  - code blocks, commands, and technical names were not altered accidentally.
- Commit in batches of 10 files unless the user changes the batch size.
- Do not check links until the whole locale is translated.
- Do not start the next locale until the current locale is fully translated and
  link-checked.

## Workflow

1. Pick the next untranslated English source file.
2. Create the localized copy under the target locale.
3. Translate only prose; keep technical tokens stable.
4. Fix any path depth changes for images and diagrams.
5. Verify the file against the English original, but do not check or fix links.
6. Mark the file as done in this checklist.
7. When the batch limit is reached, commit the batch.
8. After the whole locale is translated, run the locale-wide link checker once
   and fix only the reported issues.
9. Verify the file count and file-name set match the English source 1:1.

## Progress

### Scenario documentation extension

#### Planned: aquarium-light profile support

- [ ] Implement a reusable multichannel aquarium-light profile in the portal and firmware.
- [ ] Support saved channel curves, a daily schedule, and gradual sunrise/sunset transitions.
- [ ] Add an acclimation mode that starts below the target intensity and increases it over a configured period.
- [ ] Keep the profile generic: users define their own channel names, curve values, and target intensity.
- [ ] Create the practical multichannel aquarium-light project guide after the profile is available.

- [x] English: `projects/sensor-display.md`
- [x] English: `assets/diagrams/sensor-display-flow.svg`
- [x] English: `assets/diagrams/sensor-display-oled-preview.svg`
- [x] Russian: `projects/sensor-display.md`
- [x] Russian: `assets/diagrams/ru/i2c-wiring.svg`
- [x] Russian: `assets/diagrams/ru/sensor-display-flow.svg`
- [x] Russian: `assets/diagrams/ru/sensor-display-oled-preview.svg`
- [x] Ukrainian: `projects/sensor-display.md`
- [x] Ukrainian: `assets/diagrams/uk/i2c-wiring.svg`
- [x] Ukrainian: `assets/diagrams/uk/sensor-display-flow.svg`
- [x] Ukrainian: `assets/diagrams/uk/sensor-display-oled-preview.svg`
- [x] German: `projects/sensor-display.md`
- [x] German: `assets/diagrams/de/i2c-wiring.svg`
- [x] German: `assets/diagrams/de/sensor-display-flow.svg`
- [x] German: `assets/diagrams/de/sensor-display-oled-preview.svg`
- [x] German: `assets/screenshots/de/sensor-display-placeholder-builder.svg`
- [x] Spanish: `projects/sensor-display.md`
- [x] Spanish: `assets/diagrams/es/i2c-wiring.svg`
- [x] Spanish: `assets/diagrams/es/sensor-display-flow.svg`
- [x] Spanish: `assets/diagrams/es/sensor-display-oled-preview.svg`
- [x] Spanish: `assets/screenshots/es/sensor-display-placeholder-builder.svg`
- [x] French: `projects/sensor-display.md`
- [x] French: `assets/diagrams/fr/i2c-wiring.svg`
- [x] French: `assets/diagrams/fr/sensor-display-flow.svg`
- [x] French: `assets/diagrams/fr/sensor-display-oled-preview.svg`
- [x] French: `assets/screenshots/fr/sensor-display-placeholder-builder.svg`
- [x] Italian: `projects/sensor-display.md`
- [x] Italian: `assets/diagrams/it/i2c-wiring.svg`
- [x] Italian: `assets/diagrams/it/sensor-display-flow.svg`
- [x] Italian: `assets/diagrams/it/sensor-display-oled-preview.svg`
- [x] Italian: `assets/screenshots/it/sensor-display-placeholder-builder.svg`
- [x] English: `projects/scheduled-relay.md`
- [x] English: `assets/diagrams/scheduled-relay-flow.svg`
- [x] Russian: `projects/scheduled-relay.md`
- [x] Russian: `assets/diagrams/ru/scheduled-relay-flow.svg`
- [x] Ukrainian: `projects/scheduled-relay.md`
- [x] Ukrainian: `assets/diagrams/uk/scheduled-relay-flow.svg`
- [x] German: `projects/scheduled-relay.md`
- [x] German: `assets/diagrams/de/scheduled-relay-flow.svg`
- [x] Spanish: `projects/scheduled-relay.md`
- [x] Spanish: `assets/diagrams/es/scheduled-relay-flow.svg`
- [x] French: `projects/scheduled-relay.md`
- [x] French: `assets/diagrams/fr/scheduled-relay-flow.svg`
- [x] Italian: `projects/scheduled-relay.md`
- [x] Italian: `assets/diagrams/it/scheduled-relay-flow.svg`
- [x] English: `projects/temperature-monitor.md`
- [x] English: `assets/diagrams/temperature-monitor-flow.svg`
- [x] Russian: `projects/temperature-monitor.md`
- [x] Russian: `assets/diagrams/ru/temperature-monitor-flow.svg`
- [x] Ukrainian: `projects/temperature-monitor.md`
- [x] Ukrainian: `assets/diagrams/uk/temperature-monitor-flow.svg`
- [x] German: `projects/temperature-monitor.md`
- [x] German: `assets/diagrams/de/temperature-monitor-flow.svg`
- [x] Spanish: `projects/temperature-monitor.md`
- [x] Spanish: `assets/diagrams/es/temperature-monitor-flow.svg`
- [x] French: `projects/temperature-monitor.md`
- [x] French: `assets/diagrams/fr/temperature-monitor-flow.svg`
- [x] Italian: `projects/temperature-monitor.md`
- [x] Italian: `assets/diagrams/it/temperature-monitor-flow.svg`
- [x] English: `guides/how-gekko-works.md`
- [x] English: `projects/index.md`
- [x] English: `projects/thermostat-with-relay.md`
- [x] English: `assets/diagrams/thermostat-project-flow.svg`
- [x] Russian: `guides/how-gekko-works.md`
- [x] Russian: `projects/index.md`
- [x] Russian: `projects/thermostat-with-relay.md`
- [x] Russian: `assets/diagrams/ru/thermostat-project-flow.svg`
- [x] Ukrainian: `guides/how-gekko-works.md`
- [x] Ukrainian: `projects/index.md`
- [x] Ukrainian: `projects/thermostat-with-relay.md`
- [x] Ukrainian: `assets/diagrams/uk/thermostat-project-flow.svg`
- [x] German: `guides/how-gekko-works.md`
- [x] German: `projects/index.md`
- [x] German: `projects/thermostat-with-relay.md`
- [x] German: `assets/diagrams/de/thermostat-project-flow.svg`
- [x] Spanish: `guides/how-gekko-works.md`
- [x] Spanish: `projects/index.md`
- [x] Spanish: `projects/thermostat-with-relay.md`
- [x] Spanish: `assets/diagrams/es/thermostat-project-flow.svg`
- [x] French scenario pages and localized diagram
- [x] Italian scenario pages and localized diagram

### Ukrainian locale

#### Batch 1

- [x] `docs-site/src/content/docs/uk/index.mdx`
- [x] `docs-site/src/content/docs/uk/getting-started/what-is-gekko.md`
- [x] `docs-site/src/content/docs/uk/getting-started/hardware.md`
- [x] `docs-site/src/content/docs/uk/getting-started/flashing.mdx`
- [x] `docs-site/src/content/docs/uk/getting-started/first-boot-wifi.md`
- [x] `docs-site/src/content/docs/uk/getting-started/portal-tour.mdx`
- [x] `docs-site/src/content/docs/uk/getting-started/first-device.mdx`
- [x] `docs-site/src/content/docs/uk/guides/backup-restore.md`
- [x] `docs-site/src/content/docs/uk/guides/devices-and-dependencies.md`
- [x] `docs-site/src/content/docs/uk/guides/displays.md`

#### Batch 2

- [x] `docs-site/src/content/docs/uk/reference/devices/index.md`
- [x] `docs-site/src/content/docs/uk/reference/devices/gpio-switch.md`
- [x] `docs-site/src/content/docs/uk/reference/devices/i2c-bus.md`
- [x] `docs-site/src/content/docs/uk/reference/devices/onewire-bus.md`
- [x] `docs-site/src/content/docs/uk/reference/devices/schedule.md`
- [x] `docs-site/src/content/docs/uk/reference/devices/thermostat.md`
- [x] `docs-site/src/content/docs/uk/reference/devices/ds18b20.md`
- [x] `docs-site/src/content/docs/uk/reference/devices/htu21.md`
- [x] `docs-site/src/content/docs/uk/reference/devices/ntc-thermistor.md`
- [x] `docs-site/src/content/docs/uk/reference/devices/port-expanders.md`

#### Batch 3

- [x] `docs-site/src/content/docs/uk/guides/mqtt-home-assistant.md`
- [x] `docs-site/src/content/docs/uk/guides/ota-updates.md`
- [x] `docs-site/src/content/docs/uk/guides/schedules-and-automation.md`
- [x] `docs-site/src/content/docs/uk/reference/devices/analog-inputs.md`
- [x] `docs-site/src/content/docs/uk/reference/devices/analog-outputs.md`
- [x] `docs-site/src/content/docs/uk/reference/devices/dosing-pump.md`
- [x] `docs-site/src/content/docs/uk/reference/devices/spi-bus.md`
- [x] `docs-site/src/content/docs/uk/reference/faq.md`
- [x] `docs-site/src/content/docs/uk/reference/rest-api.md`

#### Status

- [x] Ukrainian locale completed
- [x] Ukrainian page, image, locale, and anchor links pass `pnpm check:links`

### Next locale

- [ ] Start the next locale from the translated English source set
- [ ] Keep image and diagram paths correct for the locale nesting level
- [ ] Verify every file before marking it complete
- [ ] Commit in batches of 10 unless the user changes the batch size

### Russian locale

#### Batch 1

- [x] `docs-site/src/content/docs/ru/index.mdx`
- [x] `docs-site/src/content/docs/ru/getting-started/what-is-gekko.md`
- [x] `docs-site/src/content/docs/ru/getting-started/hardware.md`
- [x] `docs-site/src/content/docs/ru/getting-started/first-boot-wifi.md`
- [x] `docs-site/src/content/docs/ru/getting-started/portal-tour.mdx`
- [x] `docs-site/src/content/docs/ru/getting-started/first-device.mdx`
- [x] `docs-site/src/content/docs/ru/guides/backup-restore.md`
- [x] `docs-site/src/content/docs/ru/guides/devices-and-dependencies.md`
- [x] `docs-site/src/content/docs/ru/guides/displays.md`
- [x] `docs-site/src/content/docs/ru/guides/mqtt-home-assistant.md`
- [x] `docs-site/src/content/docs/ru/reference/faq.md`
- [x] `docs-site/src/content/docs/ru/reference/rest-api.md`
- [x] `docs-site/src/content/docs/ru/reference/devices/index.md`
- [x] `docs-site/src/content/docs/ru/reference/devices/gpio-switch.md`
- [x] `docs-site/src/content/docs/ru/reference/devices/ds18b20.md`
- [x] `docs-site/src/content/docs/ru/reference/devices/i2c-bus.md`
- [x] `docs-site/src/content/docs/ru/reference/devices/onewire-bus.md`
- [x] `docs-site/src/content/docs/ru/reference/devices/schedule.md`
- [x] `docs-site/src/content/docs/ru/reference/devices/analog-outputs.md`
- [x] `docs-site/src/content/docs/ru/reference/devices/port-expanders.md`
- [x] `docs-site/src/content/docs/ru/reference/devices/thermostat.md`
- [x] `docs-site/src/content/docs/ru/reference/devices/analog-inputs.md`
- [x] `docs-site/src/content/docs/ru/reference/devices/dosing-pump.md`
- [x] `docs-site/src/content/docs/ru/reference/devices/htu21.md`
- [x] `docs-site/src/content/docs/ru/reference/devices/ntc-thermistor.md`
- [x] `docs-site/src/content/docs/ru/reference/devices/spi-bus.md`
- [x] `docs-site/src/content/docs/ru/getting-started/flashing.mdx`
- [x] `docs-site/src/content/docs/ru/guides/schedules-and-automation.md`
- [x] `docs-site/src/content/docs/ru/guides/ota-updates.md`

### German locale

#### Batch 1

- [x] `docs-site/src/content/docs/de/index.mdx`
- [x] `docs-site/src/content/docs/de/getting-started/what-is-gekko.md`
- [x] `docs-site/src/content/docs/de/getting-started/hardware.md`
- [x] `docs-site/src/content/docs/de/getting-started/flashing.mdx`
- [x] `docs-site/src/content/docs/de/getting-started/first-boot-wifi.md`
- [x] `docs-site/src/content/docs/de/getting-started/portal-tour.mdx`
- [x] `docs-site/src/content/docs/de/getting-started/first-device.mdx`

#### Batch 2

- [x] `docs-site/src/content/docs/de/guides/backup-restore.md`
- [x] `docs-site/src/content/docs/de/guides/devices-and-dependencies.md`
- [x] `docs-site/src/content/docs/de/guides/displays.md`
- [x] `docs-site/src/content/docs/de/guides/mqtt-home-assistant.md`
- [x] `docs-site/src/content/docs/de/guides/ota-updates.md`
- [x] `docs-site/src/content/docs/de/guides/schedules-and-automation.md`

#### Batch 3

- [x] `docs-site/src/content/docs/de/reference/devices/index.md`
- [x] `docs-site/src/content/docs/de/reference/faq.md`
- [x] `docs-site/src/content/docs/de/reference/rest-api.md`
- [x] `docs-site/src/content/docs/de/reference/devices/gpio-switch.md`
- [x] `docs-site/src/content/docs/de/reference/devices/ds18b20.md`
- [x] `docs-site/src/content/docs/de/reference/devices/thermostat.md`
- [x] `docs-site/src/content/docs/de/reference/devices/spi-bus.md`
- [x] `docs-site/src/content/docs/de/reference/devices/onewire-bus.md`
- [x] `docs-site/src/content/docs/de/reference/devices/port-expanders.md`
- [x] `docs-site/src/content/docs/de/reference/devices/schedule.md`

#### Batch 4

- [x] `docs-site/src/content/docs/de/reference/devices/analog-inputs.md`
- [x] `docs-site/src/content/docs/de/reference/devices/analog-outputs.md`
- [x] `docs-site/src/content/docs/de/reference/devices/dosing-pump.md`
- [x] `docs-site/src/content/docs/de/reference/devices/htu21.md`
- [x] `docs-site/src/content/docs/de/reference/devices/i2c-bus.md`
- [x] `docs-site/src/content/docs/de/reference/devices/ntc-thermistor.md`

#### Status

- [x] German locale completed
- [x] German page, image, locale, and anchor links pass `pnpm check:links`
- [x] German file count and file-name set match the English source 1:1

### Spanish locale

#### Batch 1

- [x] `docs-site/src/content/docs/es/index.mdx`
- [x] `docs-site/src/content/docs/es/getting-started/what-is-gekko.md`
- [x] `docs-site/src/content/docs/es/getting-started/hardware.md`
- [x] `docs-site/src/content/docs/es/getting-started/flashing.mdx`
- [x] `docs-site/src/content/docs/es/getting-started/first-boot-wifi.md`
- [x] `docs-site/src/content/docs/es/getting-started/portal-tour.mdx`
- [x] `docs-site/src/content/docs/es/getting-started/first-device.mdx`

#### Batch 2

- [x] `docs-site/src/content/docs/es/guides/backup-restore.md`
- [x] `docs-site/src/content/docs/es/guides/devices-and-dependencies.md`
- [x] `docs-site/src/content/docs/es/guides/displays.md`
- [x] `docs-site/src/content/docs/es/guides/mqtt-home-assistant.md`
- [x] `docs-site/src/content/docs/es/guides/ota-updates.md`
- [x] `docs-site/src/content/docs/es/guides/schedules-and-automation.md`

#### Batch 3

- [x] `docs-site/src/content/docs/es/reference/devices/index.md`
- [x] `docs-site/src/content/docs/es/reference/faq.md`
- [x] `docs-site/src/content/docs/es/reference/rest-api.md`
- [x] `docs-site/src/content/docs/es/reference/devices/gpio-switch.md`
- [x] `docs-site/src/content/docs/es/reference/devices/ds18b20.md`
- [x] `docs-site/src/content/docs/es/reference/devices/thermostat.md`
- [x] `docs-site/src/content/docs/es/reference/devices/spi-bus.md`
- [x] `docs-site/src/content/docs/es/reference/devices/onewire-bus.md`
- [x] `docs-site/src/content/docs/es/reference/devices/port-expanders.md`
- [x] `docs-site/src/content/docs/es/reference/devices/schedule.md`

#### Batch 4

- [x] `docs-site/src/content/docs/es/reference/devices/analog-inputs.md`
- [x] `docs-site/src/content/docs/es/reference/devices/analog-outputs.md`
- [x] `docs-site/src/content/docs/es/reference/devices/dosing-pump.md`
- [x] `docs-site/src/content/docs/es/reference/devices/htu21.md`
- [x] `docs-site/src/content/docs/es/reference/devices/i2c-bus.md`
- [x] `docs-site/src/content/docs/es/reference/devices/ntc-thermistor.md`

#### Status

- [x] Spanish locale completed
- [x] Spanish page, image, locale, and anchor links pass `pnpm check:links`
- [x] Spanish file count and file-name set match the English source 1:1

### French locale

#### Batch 1

- [x] `docs-site/src/content/docs/fr/index.mdx`
- [x] `docs-site/src/content/docs/fr/getting-started/what-is-gekko.md`
- [x] `docs-site/src/content/docs/fr/getting-started/hardware.md`
- [x] `docs-site/src/content/docs/fr/getting-started/flashing.mdx`
- [x] `docs-site/src/content/docs/fr/getting-started/first-boot-wifi.md`
- [x] `docs-site/src/content/docs/fr/getting-started/portal-tour.mdx`
- [x] `docs-site/src/content/docs/fr/getting-started/first-device.mdx`
- [x] `docs-site/src/content/docs/fr/guides/backup-restore.md`
- [x] `docs-site/src/content/docs/fr/guides/devices-and-dependencies.md`
- [x] `docs-site/src/content/docs/fr/guides/displays.md`

#### Batch 2

- [x] `docs-site/src/content/docs/fr/reference/devices/index.md`
- [x] `docs-site/src/content/docs/fr/reference/devices/onewire-bus.md`
- [x] `docs-site/src/content/docs/fr/reference/devices/i2c-bus.md`
- [x] `docs-site/src/content/docs/fr/reference/devices/spi-bus.md`
- [x] `docs-site/src/content/docs/fr/reference/devices/ds18b20.md`
- [x] `docs-site/src/content/docs/fr/reference/devices/schedule.md`
- [x] `docs-site/src/content/docs/fr/reference/devices/thermostat.md`
- [x] `docs-site/src/content/docs/fr/guides/mqtt-home-assistant.md`
- [x] `docs-site/src/content/docs/fr/guides/ota-updates.md`
- [x] `docs-site/src/content/docs/fr/guides/schedules-and-automation.md`

#### Batch 3

- [x] `docs-site/src/content/docs/fr/reference/devices/gpio-switch.md`
- [x] `docs-site/src/content/docs/fr/reference/devices/port-expanders.md`
- [x] `docs-site/src/content/docs/fr/reference/devices/analog-outputs.md`
- [x] `docs-site/src/content/docs/fr/reference/devices/analog-inputs.md`
- [x] `docs-site/src/content/docs/fr/reference/devices/ntc-thermistor.md`
- [x] `docs-site/src/content/docs/fr/reference/devices/htu21.md`
- [x] `docs-site/src/content/docs/fr/reference/devices/dosing-pump.md`
- [x] `docs-site/src/content/docs/fr/reference/faq.md`
- [x] `docs-site/src/content/docs/fr/reference/rest-api.md`

#### Status

- [x] French locale completed
- [x] French page, image, locale, and anchor links pass `pnpm check:links`
- [x] French file count and file-name set match the English source 1:1

### Italian locale

#### Batch 1

- [x] `docs-site/src/content/docs/it/index.mdx`
- [x] `docs-site/src/content/docs/it/getting-started/what-is-gekko.md`
- [x] `docs-site/src/content/docs/it/getting-started/hardware.md`
- [x] `docs-site/src/content/docs/it/getting-started/flashing.mdx`
- [x] `docs-site/src/content/docs/it/getting-started/first-boot-wifi.md`
- [x] `docs-site/src/content/docs/it/getting-started/portal-tour.mdx`
- [x] `docs-site/src/content/docs/it/getting-started/first-device.mdx`
- [x] `docs-site/src/content/docs/it/guides/backup-restore.md`
- [x] `docs-site/src/content/docs/it/guides/devices-and-dependencies.md`
- [x] `docs-site/src/content/docs/it/guides/displays.md`

#### Batch 2

- [x] `docs-site/src/content/docs/it/reference/devices/index.md`
- [x] `docs-site/src/content/docs/it/reference/devices/onewire-bus.md`
- [x] `docs-site/src/content/docs/it/reference/devices/i2c-bus.md`
- [x] `docs-site/src/content/docs/it/reference/devices/spi-bus.md`
- [x] `docs-site/src/content/docs/it/reference/devices/ds18b20.md`
- [x] `docs-site/src/content/docs/it/reference/devices/schedule.md`
- [x] `docs-site/src/content/docs/it/reference/devices/thermostat.md`
- [x] `docs-site/src/content/docs/it/guides/mqtt-home-assistant.md`
- [x] `docs-site/src/content/docs/it/guides/ota-updates.md`
- [x] `docs-site/src/content/docs/it/guides/schedules-and-automation.md`

#### Batch 3

- [x] `docs-site/src/content/docs/it/reference/devices/gpio-switch.md`
- [x] `docs-site/src/content/docs/it/reference/devices/port-expanders.md`
- [x] `docs-site/src/content/docs/it/reference/devices/analog-outputs.md`
- [x] `docs-site/src/content/docs/it/reference/devices/analog-inputs.md`
- [x] `docs-site/src/content/docs/it/reference/devices/ntc-thermistor.md`
- [x] `docs-site/src/content/docs/it/reference/devices/htu21.md`
- [x] `docs-site/src/content/docs/it/reference/devices/dosing-pump.md`
- [x] `docs-site/src/content/docs/it/reference/faq.md`
- [x] `docs-site/src/content/docs/it/reference/rest-api.md`

#### Status

- [x] Italian locale files translated
- [x] Italian page, image, locale, and anchor links pass `pnpm check:links`
- [x] Italian file count and file-name set match the English source 1:1

### Short version

1. Translate all files in the locale.
2. Run link check.

### Current state

- Supported translated locales: `uk`, `ru`, `de`, `es`, `fr`, `it`
- Files per locale: 29
- Remaining untranslated supported locales: none
- Next new locale, if added later, should use the 29-file checklist pattern from this plan

### Automated verification

- [x] Link fixer/checker has isolated positive and negative tests
- [x] English, Ukrainian, and Russian documents pass the source-level link check
- [x] Astro production build passes the Starlight internal-link validator
- [x] Locale file counts and names can be checked 1:1 against the English tree
- [x] File count / name check command:

  ```sh
  python3 - <<'PY'
  from pathlib import Path

  root = Path('docs-site/src/content/docs')
  english = {
      str(p.relative_to(root))
      for p in root.rglob('*')
      if p.is_file() and len(p.parts) > 4 and p.parts[4] not in ('de', 'uk', 'ru')
  }

  for locale in ('de', 'uk', 'ru'):
      localized_root = root / locale
      localized = {
          str(p.relative_to(localized_root))
          for p in localized_root.rglob('*')
          if p.is_file()
      }
      print(locale, 'OK' if english == localized else 'MISMATCH')
  PY
  ```

## Link Check Script

Use the link checker after the whole locale is translated.

### Commands

- `pnpm check:links` runs the checker in read-only mode over `src/content/docs`.
- `pnpm fix:links` runs the same checker in write mode and applies only verified rewrites.
- `pnpm test:links` runs the isolated script tests.

### What the script checks

- Markdown links and images
- Reference-style link definitions
- `link` and `file` values in frontmatter
- Static MDX `href` and `src` attributes
- Internal page targets, locale-specific page targets, asset paths, public files, and heading anchors

### What the script changes

- Rewrites `/gekko/...` page links to the current locale only after the localized page exists.
- Rewrites asset paths only after resolving the real file under `src/assets`.
- Rewrites anchors only when the translated heading structure still matches the source target.
- Leaves the file unchanged in `--check` mode.

### What the script ignores

- External URLs such as `https://...`
- Explicit service routes like `/gekko/install/`

### Failure cases

- Missing page
- Missing localized page
- Missing asset
- Missing anchor
- Anchor structure mismatch between source and translation
- Ambiguous `.md` versus `.mdx` targets
- Unsupported root routes

### Expected workflow

1. Translate one file.
2. Continue until the whole locale is translated.
3. Run `pnpm check:links` on the locale subtree.
4. Fix any reported page, image, or anchor issue.
5. Run the checker again.
6. Mark the locale done only after the final check is clean.

### Russian status

- [x] Russian locale completed
- [x] No Russian files remain open in this plan
