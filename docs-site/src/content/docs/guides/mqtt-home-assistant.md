---
title: MQTT & Home Assistant
description: Flip one toggle per device and it appears in Home Assistant — switches, sensors, and thermostats, controllable from the HA UI.
sidebar:
  order: 4
---

Gekko speaks **Home Assistant MQTT discovery**: connect it to your MQTT broker
once, then publish any device with a single toggle — and it appears in Home
Assistant on its own, with the right entity type, name, and icon. No YAML, no
manual entity configuration.

## What you get

![Gekko devices appearing in Home Assistant as switch, sensors, and climate entities](../../../assets/diagrams/ha-entities.svg)

Each published Gekko device becomes a native HA entity, and control flows both
ways in real time:

| Gekko device | In Home Assistant | You can |
| --- | --- | --- |
| GPIO / port-expander / auto switch | `switch` | toggle it from any HA dashboard, use it in automations |
| Analog output (fade / scheduled) | `light` (brightness) | dim it from HA, drop it into scenes |
| Pixel strip | `light` (brightness) | control an addressable strip's power and brightness |
| Pixel effect solid | `light` (RGB) | pick the strip's color from HA's color wheel |
| Pixel effect alert | `binary_sensor` | know from HA whether the alert is currently blinking |
| DS18B20, NTC thermistor | `sensor` | chart history, trigger automations on temperature |
| HTU21 | two `sensor`s (temperature + humidity) | same, independently |
| Binary sensor | `binary_sensor` | leak/door alerts through HA notifications |
| Thermostat | `climate` | change mode and setpoint from HA's thermostat card |

So your aquarium light can join HA scenes, the leak sensor can push a phone
notification, and the thermostat shows up next to your home's climate
controls — while everything still runs locally on the ESP32 even if HA is
down.

## Setup

1. **Connect the broker (once).** On the portal's **MQTT / Home Assistant**
   page enter your broker host, port, and credentials (TLS supported), and
   switch **Enable MQTT** on. Settings changes apply with a clean reconnect —
   no reboot. MQTT only connects once the device is on your WiFi as a station,
   never in setup-AP mode.

   ![MQTT broker settings page](../../../assets/screenshots/portal-mqtt.png)

2. **Make sure HA is on the same broker** with its MQTT integration's
   discovery enabled (the default).

3. **Publish devices.** Every supported device's page has a **Home Assistant**
   card — flip **Publish to Home Assistant**, optionally give it an HA-specific
   name, save:

   ![Per-device Home Assistant card with the publish toggle](../../../assets/screenshots/device-ha-card.png)

   Seconds later the device is in HA under **Settings → Devices & services →
   MQTT**, grouped under your Gekko controller. Unpublishing removes it just
   as cleanly.

## A build-time option

MQTT support is compiled into the firmware on demand (`-DWITH_HOME_ASSISTANT`
in `platformio.ini`) — firmware without it carries no MQTT code at all, which
matters on 4 MB boards. The portal is explicit about the difference:

- the **Available / Not available** chip on the MQTT page tells you whether
  this *build* has the feature;
- the **Enable MQTT** switch tells the firmware whether to actually connect
  right now.

On builds without the feature, the MQTT page shows an explanatory note and the
per-device HA cards don't render.

For the full architecture (topic scheme, adapters, TLS certificates), see
[`docs/mqtt-home-assistant.md`](https://github.com/yoreek/gekko/blob/master/docs/mqtt-home-assistant.md)
in the repository.
