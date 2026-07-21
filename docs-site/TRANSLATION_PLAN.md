# Translation plan

This file tracks the translation workflow for `docs-site/`.

## Rules

- Translate one locale at a time.
- Work one file at a time, or in small batches only when explicitly allowed.
- Before marking a file done, verify:
  - frontmatter still matches the source structure;
  - headings are present and in the same order;
  - links still point to valid targets;
  - image and diagram paths are correct for the localized nesting level;
  - code blocks, commands, and technical names were not altered accidentally.
- Commit in batches of 10 files unless the user changes the batch size.
- Do not advance to the next locale or batch until the current batch is complete and checked.

## Workflow

1. Pick the next untranslated English source file.
2. Create the localized copy under the target locale.
3. Translate only prose; keep technical tokens stable.
4. Fix any path depth changes for images and diagrams.
5. Verify the file against the English original.
6. Mark the file as done in this checklist.
7. When the batch limit is reached, commit the batch.

## Progress

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

#### Status

- [x] Russian locale completed
- [x] Russian page, image, locale, and anchor links pass `pnpm check:links`

### Automated verification

- [x] Link fixer/checker has isolated positive and negative tests
- [x] English, Ukrainian, and Russian documents pass the source-level link check
- [x] Astro production build passes the Starlight internal-link validator
