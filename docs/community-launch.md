# Community Launch Notes

Use the canonical project URL in posts:
<https://github.com/yoreek/gekko>. Link to the browser installer only after
explaining that it writes a complete ESP32 image and erases existing device
data.

## Short announcement

**Gekko: configure an ESP32 controller from the browser instead of rebuilding
firmware**

I built Gekko, an open-source modular device controller for ESP32. Its firmware
contains a catalog of composable sensors, outputs, buses, schedules, dosing
pumps, thermostats, and displays. Hardware and dependencies are configured at
runtime from the ESP32-hosted web portal, so each installation does not need a
custom firmware build.

It also supports MQTT/Home Assistant discovery, OTA updates, backup/restore,
BLE or access-point WiFi provisioning, and a browser-based firmware installer.

Project: <https://github.com/yoreek/gekko>

Documentation and installer: <https://yoreek.github.io/gekko/>

I would especially appreciate feedback on the device dependency model,
supported hardware, and installation flow.

## Compact social post

Gekko is an open-source modular ESP32 controller configured entirely from its
built-in web UI: sensors, relays, PWM, schedules, automation, pumps, RTCs, and
five display types without rebuilding firmware for every installation. MQTT,
Home Assistant, OTA, backup/restore, and browser flashing are included.

<https://github.com/yoreek/gekko>

## Community-specific framing

- **[r/esp32](https://www.reddit.com/r/esp32/):** emphasize the runtime device
  registry, dependency validation, cooperative firmware flow, and reproducible
  flash bundles. The subreddit requires reading and acknowledging its rules;
  show-and-tell posts should explain the ESP32 engineering instead of posting
  only a project link.
- **[Projects made with PlatformIO](https://community.platformio.org/c/projects-made-with-platformio/14):**
  describe the PlatformIO environments, native test suite, CI packaging, and
  why the same firmware image supports different installations.
- **[Home Assistant — Share your Projects](https://community.home-assistant.io/c/projects/9):**
  lead with local-first operation, optional MQTT discovery, and the fact that
  the controller remains functional without Home Assistant.
- **Aquarium, greenhouse, and maker communities:** show one concrete build and
  its wiring graph instead of posting only a general feature list.
- **[Hackaday.io](https://hackaday.io/):** create a project page with the
  dashboard image, architecture overview, build instructions, and a
  development log for each release.

## Publishing checklist

1. Publish a tagged GitHub Release with the default and BLE firmware ZIPs.
2. Upload `.github/social-preview.png` in **Settings → General → Social
   preview**.
3. Include one screenshot and one concrete use case in each post.
4. Adapt the opening paragraph to the community; do not cross-post identical
   text everywhere at once.
5. Answer setup questions and turn recurring questions into documentation.

Regenerate the social preview after a major UI change:

```sh
pnpm --dir docs-site exec node scripts/render-social-preview.mjs
```
