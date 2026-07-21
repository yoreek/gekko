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
8. Before closing a locale, verify the file count and file-name set match the English source 1:1.

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

Use the link checker before marking a translated file done.

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
2. Run `pnpm check:links` on that file or locale subtree.
3. Fix any reported page, image, or anchor issue.
4. Run the checker again.
5. Mark the file done only after the check is clean.

### Russian status

- [x] Russian locale completed
- [x] No Russian files remain open in this plan
