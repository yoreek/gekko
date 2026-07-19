---
title: MQTT & Home Assistant
description: Connect Gekko to an MQTT broker and let Home Assistant discover your devices automatically.
sidebar:
  order: 4
---

Gekko can connect to an MQTT broker and publish **Home Assistant MQTT
discovery**, so your Gekko devices appear in Home Assistant automatically —
switches as toggles, sensors as readings, thermostats as climate entities.

## An optional, compile-time feature

MQTT support is **off by default** and is a build-time option: firmware
compiled without it contains no MQTT code at all (smaller flash — a real
concern on 4 MB boards). To enable it, build from source with
`-DWITH_HOME_ASSISTANT` uncommented in `platformio.ini`.

There are two independent "on" switches — don't confuse them:

1. **Compiled in** — whether this firmware build has the feature at all. The
   portal's MQTT page shows an "Available"/"Not available" chip for this; on
   builds without the feature the page shows an explanatory note instead of the
   settings form.
2. **Runtime toggle** — the "Enable MQTT" switch in the settings form: whether
   the device should actually connect to your broker right now. Settings
   changes (host, credentials, TLS) apply with a clean reconnect, no reboot.

MQTT only connects once the device has a real station connection to your
network — never while in the setup-AP provisioning state.

## What Home Assistant sees

Each Gekko device maps to one or more HA entities via discovery:

| Gekko device | Home Assistant entity |
| --- | --- |
| GPIO switch / port-expander switch / auto switch | `switch` (controllable) |
| DS18B20, NTC thermistor | `sensor` (temperature, read-only) |
| HTU21 | two `sensor`s — temperature and humidity |
| Binary sensor | `binary_sensor` |
| Thermostat | `climate` — mode, setpoint, current temperature, action |

Commands flow back too: toggling the HA switch flips the real output, and
changing the climate setpoint updates the thermostat's validated config. Each
entity ships with a sensible `mdi:*` icon.

## Setup

1. On the portal's **MQTT / Home Assistant** page, enter your broker host,
   port, and credentials, and switch **Enable MQTT** on.
2. Make sure Home Assistant's MQTT integration is connected to the same
   broker with discovery enabled (the default).
3. Your devices appear under **Settings → Devices & services → MQTT** within a
   few seconds, grouped under the Gekko controller.

For the full architecture (topic scheme, adapters, TLS notes), see
[`docs/mqtt-home-assistant.md`](https://github.com/yoreek/gekko/blob/master/docs/mqtt-home-assistant.md)
in the repository.
