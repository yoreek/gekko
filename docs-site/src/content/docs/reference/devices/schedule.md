---
title: Schedule
description: Reference for Gekko's schedule device type — minute-precision time-of-day and weekday rules.
sidebar:
  order: 8
---

`schedule` holds a set of time rules and answers one question: *is this
schedule active right now?* It has no output of its own — attach it as a
condition to an [auto switch](/gekko/guides/schedules-and-automation/) (or a
dosing pump's scheduling) to make something happen.

![Schedule rule editor](../../../../assets/screenshots/device-schedule.png)

## Dependencies

None. Other devices depend on the schedule, not the other way around.

## Configuration

Up to **4 rules**, OR-ed together — the schedule is active when any enabled
rule matches. Each rule:

| Field | Meaning |
| --- | --- |
| Weekdays | Which days of the week the rule applies |
| Start / end time | The active window, in minutes of the day (minute precision — there are no seconds) |
| Mode | **Always on** — active for the whole window; **Interval** — split the window into N equal slices, active for the first M minutes of each |

Interval mode covers periodic tasks: e.g. a 08:00–20:00 window with 12
intervals and 5 minutes duration runs a circulation pump 5 minutes every hour.

## Time & clock

Rules are evaluated against the device's own clock and configured timezone
(with DST handled automatically). Until the clock is plausible — NTP synced or
a DS3231 RTC present — the schedule reports itself as not valid and dependent
devices keep their outputs safe.

The portal's editor shows an on/off preview and the next transition, computed
in your browser from the same rules; it is labelled an estimate because your
browser's clock and timezone may differ from the device's.

## Provides

- **condition** — for auto switches and chained automations.
- **schedule** — for devices that consume schedules directly.
