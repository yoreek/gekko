---
title: Displays & layout designer
description: Drive SSD1306 OLED and ST7735 TFT displays with Gekko's visual layout designer and live metric placeholders.
sidebar:
  order: 3
---

Gekko drives **SSD1306** I2C OLED displays and **ST7735** SPI TFT displays, and
the portal includes a **visual layout designer** — you compose what the screen
shows from pages and widgets, live in the browser, with a preview.

## Setting up a display

1. Create the bus device first: an **I2C bus** (SDA/SCL pins) for SSD1306, or
   an **SPI bus** for ST7735.
2. Create the display device and select that bus as its dependency (plus the
   I2C address or the TFT's control pins).
3. Open the device and click **Design** to enter the layout designer.

![Display layout designer](../../../assets/screenshots/portal-display-designer.png)

## Pages and widgets

A layout is a set of **pages**; each page holds positioned **widgets** (text
and more). The designer shows a live preview rendered with the same fonts and
metrics the firmware uses, so what you see is what the panel draws. Layouts are
saved on the device and are included in
[backup bundles](/gekko/guides/backup-restore/).

## Live values: metric placeholders

Text widgets can mix static text with **placeholders** resolved at render time:

```text
Room {{dev.670845748.temperature | fixed:1}}
IP {{system.wifi.station_ip}}
Now {{system.time | format:HH:mm}}
Up {{system.uptime}}
```

Placeholder forms:

- `{{dev.<deviceId>.<metricKey>}}` — a metric from any device (temperature,
  state, …). The designer has a placeholder builder listing everything
  available.
- `{{system.<metricKey>}}` — system metrics such as `time` (wall clock) and
  `uptime` (elapsed since boot).
- `{{system.wifi.<metricKey>}}` — WiFi metrics such as `station_ip`.

Optional filters follow after `|`:

| Filter | Example | Effect |
| --- | --- | --- |
| `fixed:N` | `{{dev.123.temperature \| fixed:1}}` | Decimal formatting with N digits |
| `format:pattern` | `{{system.time \| format:EEEE HH:mm}}` | Date/time pattern (`YYYY MM DD HH mm ss EEEE`; `[literal]` text in brackets) |
| `upper` / `lower` / `trim` | `{{system.wifi.station_ip \| upper}}` | Text transforms |

A placeholder that cannot be resolved renders as empty text instead of breaking
the whole widget, so a temporarily missing sensor never blanks your screen.

Devices referenced by placeholders become real registry dependencies of the
display — the registry will warn you before deleting a sensor a display still
shows.
