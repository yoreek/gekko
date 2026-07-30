---
title: Displays & layout designer
description: Configure pixel, character, and seven-segment displays with Gekko's visual layout designer and live metric placeholders.
sidebar:
  order: 3
---

Gekko supports five display types through one shared **visual layout
designer**. You compose pages and widgets in the browser with a preview; the
available coordinates and widgets adapt to the selected display.

| Type | Hardware | Layout coordinates | Widgets |
| --- | --- | --- | --- |
| `ssd1306` | I2C monochrome OLED | Pixels | Text, shapes, and bitmaps |
| `st7735` | SPI color TFT | Pixels | Text, shapes, and RGB565 bitmaps |
| `lcd1602` | HD44780 16 × 2 through PCF857x | Character cells | Character |
| `lcd2004` | HD44780 20 × 4 through PCF857x | Character cells | Character |
| `tm1637` | Four-digit seven-segment module | Digit positions | Digital |

## Setting up a display

1. Create the required infrastructure first:
   - an [**I2C bus**](/gekko/reference/devices/i2c-bus/) for SSD1306;
   - an [**SPI bus**](/gekko/reference/devices/spi-bus/) for ST7735;
   - a PCF8574/PCF8575 [**port expander**](/gekko/reference/devices/port-expanders/)
     for LCD1602/LCD2004;
   - nothing for TM1637: it drives its CLK and DIO pins directly.
2. Create the display and select those devices as its dependencies, then set
   its address, wiring channels, control pins, brightness, or rotation as
   applicable.
3. Open the device and click **Design** to enter the layout designer.

![Display layout designer](../../../assets/screenshots/portal-display-designer.png)

## Pages and widgets

A layout is a set of **pages**; each page holds positioned **widgets**. Pixel
displays use pixel coordinates, LCD1602/LCD2004 use character-cell coordinates,
and TM1637 uses digit positions. The designer permits only the widget types
supported by that display and shows a matching live preview. Layouts are saved
on the device and are included in
[backup bundles](/gekko/guides/backup-restore/).

## Live values: metric placeholders

Text, Character, and Digital widgets can mix static text with **placeholders**
resolved at render time. Building a status screen is just writing a few lines
of template text:

![Widget text with placeholders on the left, the rendered OLED output with live values on the right](../../../assets/diagrams/display-placeholders.svg)

You don't have to memorize the syntax — the designer's **placeholder builder**
lists every available metric of every device with a live preview value, and
inserts the placeholder for you. Typed placeholders are validated on every
keystroke. More examples:

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

A placeholder that cannot be resolved renders as `N/A` instead of breaking
the whole widget, so a temporarily missing sensor never blanks your screen —
you'll see `N/A` in its place instead of a silent gap.

Devices referenced by placeholders become real registry dependencies of the
display — the registry will warn you before deleting a sensor a display still
shows.
