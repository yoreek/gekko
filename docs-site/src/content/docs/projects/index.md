---
title: Projects
description: Start with an outcome, then follow the wiring, dependency graph, setup order, and verification steps.
sidebar:
  order: 0
---

These guides start with a practical result instead of a device type. Each one
shows the required hardware, the Gekko device graph, the creation order, and a
safe way to verify the result.

| Project | What you build | Main concepts |
| --- | --- | --- |
| [Temperature monitor](/gekko/projects/temperature-monitor/) | A verified DS18B20 temperature reading and history | 1-Wire, scan, sensor address, live reading |
| [Thermostat with relay](/gekko/projects/thermostat-with-relay/) | A DS18B20-controlled heater or cooler | 1-Wire, sensor, switch, two dependencies, safe state |
| [Scheduled relay](/gekko/projects/scheduled-relay/) | A relay controlled by time and weekday rules | clock, schedule, Auto Switch, safe output |
| [Sensor display](/gekko/projects/sensor-display/) | A live DS18B20 value on an SSD1306 OLED | 1-Wire, I2C, display layout, metric placeholder |
| [Dosing pump](/gekko/projects/dosing-pump/) | A calibrated liquid dose on a reliable schedule | GPIO switch, calibration, dose slots, container tracking |
| [Multichannel light](/gekko/projects/multichannel-light/) | One dimmable fixture with coordinated PWM channels | PWM outputs, curves, fades, light composer |

If you already know the technical type you need, use the
[device catalog](/gekko/reference/devices/). Otherwise, choose a project first
and use its links to reach only the relevant reference pages.
