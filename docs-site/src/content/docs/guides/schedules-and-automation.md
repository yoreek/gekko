---
title: Schedules & automation
description: Time-of-day schedules and condition-driven auto switches in Gekko.
sidebar:
  order: 2
---

Two device types work together to automate switching: a **schedule** holds
time-of-day rules and reports whether it is currently active, and an **auto
switch** drives a real switch from the logical AND of its attached conditions.

## Schedule

A [schedule device](/gekko/reference/devices/schedule/) holds up to 4 rules.
Each rule has:

- a **weekday mask** — which days it applies to;
- a **time window** — start and end minute of the day (minute precision, no
  seconds);
- a **mode**:
  - **Always on** — active for the whole window;
  - **Interval** — splits the window into equal slices and is active for the
    first N minutes of each slice (for periodic circulation, misting, etc.).

The schedule is active when **any** enabled rule matches. The portal's rule
editor shows a client-side on/off preview computed from the rules against your
browser's clock — labelled as an estimate, since the device evaluates rules
against its own clock and timezone.

:::note[Give the device a reliable clock]
Schedules refuse to act until the device's clock is plausible. Use NTP (set the
timezone on the **Time** page) or add a DS3231 RTC device so schedules survive
internet outages and reboots.
:::

## Auto switch

An auto switch wraps a real switch (GPIO or port-expander) and drives it from
up to **6 condition dependencies** — schedules, other switches, or other auto
switches — each optionally **inverted**. All conditions are ANDed: the output
is on only while every condition is satisfied. With no conditions attached, an
auto switch in Auto mode stays off.

Its mode is one of:

- **Off / On** — manual override; conditions are ignored. Toggling the switch
  from the dashboard sets exactly this.
- **Auto** — follow the conditions.
- **Paused** — temporarily off for a configured duration, then automatically
  back to **Auto**. Pause is only available from Auto mode. A reboot mid-pause
  resumes the pause with the correct remaining time.

When entering Auto (or Paused), the target switch is always forced off first,
then handed to the conditions — so a previous manual "On" never silently
sticks.

Because an auto switch itself acts as a switch and a condition, you can chain
automations: a "feeding mode" auto switch can gate several other auto switches
at once through their inverted condition slots.

## Example: aquarium light with a pause button

1. Create a **Schedule** "Light hours", rule: every day, 09:00–21:00, always
   on.
2. Create a **GPIO Switch** "Light relay" on the pin driving your light.
3. Create an **Auto Switch** "Light": target = "Light relay", condition =
   "Light hours", mode = Auto.
4. Pin "Light" to the dashboard. It now follows the schedule; tap it for a
   manual override, use **pause** during maintenance, and return it to Auto
   when done.

## Related devices

- **[Thermostat](/gekko/reference/devices/thermostat/)** — hysteresis
  temperature control driving a switch.
- **[Dosing pump](/gekko/reference/devices/dosing-pump/)** — scheduled dosing
  with calibration, container accounting, and a dose journal.
- **[Scheduled analog output](/gekko/reference/devices/analog-outputs/)** — a
  daily brightness/level curve for PWM outputs, composable into multi-channel
  fixtures.
