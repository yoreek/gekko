# SPA Localization

How to add and edit `portal-spa` i18n keys without duplicating phrases across
device forms, languages, or REST error strings. See `docs/device-scaffolding.md`
checklist item 11 ("i18n") for *when* a new key is needed when scaffolding a
device type; this doc covers *how* to write it so it doesn't drift.

## Where keys live

One file per language under `portal-spa/src/i18n/locales/` (`en.ts`, `ru.ts`,
`de.ts`, `es.ts`, `fr.ts`, `it.ts`, `uk.ts`), each exporting the same key tree.
The tree is namespaced by feature, not by component file:

- `device.type.<typeName>` — the label shown in the create-menu/type picker.
- `device.fields.<name>` — a field label, reused by every form that has that
  field.
- `device.dialog.<typeName>.<key>` — hints/notices that are genuinely specific
  to one device type's dialog (a `noDependency` message naming the exact
  dependency role it needs, for example).
- `device.dialog.display.widgetTypes.<type>` — the designer's widget-type
  dictionary; keys must stay in exact sync with the `DisplayWidgetType` union
  in `portal-spa/src/models/devices/display/layout.ts` (see "Keep dictionaries
  exhaustive" below).

## Core rule: one key per distinct phrase, not per call site

If two forms show the same label or hint for the same underlying field
concept, they reference the **same key** — never a second key holding
identical or near-identical text. `Lcd2004Fields.vue` already does this
correctly: it reuses `device.fields.lcd1602RsChannel` (and the rest of the
`lcd1602*Channel` keys) rather than declaring its own `lcd2004RsChannel`,
because an LCD2004's RS channel is the same *concept* as an LCD1602's — same
label, same hint, same validation story. Only the visible geometry (16×2 vs
20×4) differs, and that lives in a separate key/constant, not duplicated
phrasing.

The test: **is this the same field, described the same way, just used by a
sibling device type?** If yes, reuse the key. If the wording only differs by a
device name that's already visible elsewhere in the UI (dialog title, response
`typeName`), that's not a real distinction — drop the type name and make the
phrase generic instead of writing `<type>` + suffix per adapter. This mirrors
the same principle applied on the C++ side in `TypedHd44780PinDeviceApiAdapter`
and `TypedDisplayDeviceApiAdapter`: error strings like `"layout is invalid"`
live once in the shared base template, generic (no `lcd1602_pin`/`lcd2004_pin`
prefix), because the response already carries `typeName` — repeating it in the
message text was pure duplication, not information.

When you *are* about to write a new field label or hint, grep the locale file
for the phrase first:

```sh
grep -n "RS channel\|RS pin" portal-spa/src/i18n/locales/en.ts
```

If something close already exists, decide whether it's the *same concept*
(reuse the key) or a *different concept that happens to read similarly* (new
key). "RS channel" (a PCF8574 bit position, 0-7) and "RS pin" (a raw ESP32
GPIO number, 0-255) are genuinely different fields with different validation
ranges — separate keys are correct there, even though the label pattern looks
the same. Don't merge distinct concepts just because the text is similar; don't
duplicate the same concept just because it's used from a second file.

## Reusing hints within one locale file

For a hint string that's shared by more than two dialogs within a single
locale, use a local object at the top of that locale file and spread/reference
it, instead of retyping the string at every use site — `deviceDialogCommon` in
`en.ts` already does this for pin/bus hints (`gpioPinHint`, `i2cSdaHint`,
`i2cFrequencyHint`, ...):

```ts
const deviceDialogCommon = {
  gpioPinHint: 'Hardware output pin used by the switch runtime.',
  // ...
}
```

referenced later as `gpioPinHint: deviceDialogCommon.gpioPinHint`. Add to this
object rather than inlining a fourth or fifth copy of a hint that keeps
reappearing.

## Keep dictionaries exhaustive

Some i18n subtrees mirror a TypeScript union type one-to-one (widget types,
binding kinds, dependency roles, ...). When the union gains a member, the i18n
dictionary must gain a matching key **in every language**, or `vue-i18n` falls
back silently (a console warning, then the raw English string in every other
locale) — this is exactly how `device.dialog.display.widgetTypes.character`
went missing for months: the `'character'` widget type existed in
`DisplayWidgetType` since lcd1602/lcd2004 shipped, but no locale ever got a
`character:` entry in `widgetTypes`, because the two were added in different
commits without cross-checking the dictionary against the union it's supposed
to cover. When you add a member to a union that has a matching i18n
dictionary, add the key immediately, not "when the label looks wrong" —
`vue-i18n`'s fallback means it won't necessarily look wrong until someone
opens a non-English locale.

## Adding keys for a new device type

1. Before writing anything, check whether the new type is a variant of an
   existing one (different transport/geometry/pins, same field concepts —
   this is exactly the lcd1602/lcd1602_pin relationship). If so, reuse that
   type's field-label and hint keys directly; only add new keys for fields
   that don't already exist anywhere (see the RS-channel-vs-RS-pin example
   above for when a field *looks* similar but isn't).
2. Add `device.type.<typeName>` (the create-menu label) to **all seven**
   locale files in the same change — a key present in `en.ts` only means
   every other language silently falls back to English for that menu entry.
3. If the label text only differs from an existing sibling by "(direct pins)"
   or similar, keep the base noun translated consistently with the sibling
   entry (check what translation the sibling already uses for the shared
   noun) rather than re-translating from scratch and drifting.
4. Run `pnpm build` (the `vue-tsc` pass will catch a key referenced in a
   template that doesn't exist in the TS locale type) and open the designer/
   fields dialog for the new type in at least one non-English locale before
   calling it done — a missing key doesn't fail the build, it just silently
   falls back.
