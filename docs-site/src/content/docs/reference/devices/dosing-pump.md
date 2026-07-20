---
title: Dosing pump
description: What a dosing pump is, why aquarists automate dosing, and how Gekko's dosing_pump device schedules, calibrates, and tracks every millilitre.
sidebar:
  order: 9
---

## What is a dosing pump?

A dosing pump is a slow, precise liquid pump. The usual kind is **peristaltic**:
a small motor squeezes a soft silicone tube with rollers, pushing a few
millilitres per second through it. Because the liquid only ever touches the
tube, the pump can't contaminate it — and because the flow rate is steady, *run
time translates directly into millilitres*.

Aquarists (and gardeners) use them wherever a liquid must be added **little and
often**:

- **Reef tanks** — calcium, alkalinity (KH), magnesium, trace elements. Corals
  consume these continuously; daily micro-doses keep the water chemistry far
  more stable than one big weekly correction.
- **Planted tanks** — daily liquid fertilizer instead of "when I remember".
- **Ponds/greenhouses** — pH buffer, nutrients.

Manual dosing means measuring cylinders, a calendar, and inevitable missed
days. An automated dosing pump does the same job every day at the same minute —
that consistency is the whole point.

## The pieces, and how Gekko wires them together

![Dosing pump setup: container with float sensor, peristaltic pump driven through a relay, ESP32, aquarium](../../../../assets/diagrams/dosing-setup.svg)

You need four cheap parts, and each one maps to a Gekko device:

| Hardware | Gekko device | Role |
| --- | --- | --- |
| Peristaltic pump + relay/MOSFET board (orange wire above) | `gpio_switch` (or `port_expander_switch`) | The output the pump device switches on and off |
| The dosing pump itself (logic) | `dosing_pump` | Owns the schedule, calibration, container, and history |
| Bottle/canister with the solution | — (tracked by the `dosing_pump` config) | What you're dosing from |
| Optional float switch in the bottle (green wire above) | `binary_sensor` | Tells Gekko the bottle is empty regardless of the counter |

Several pumps? Create one chain per liquid — a typical reef stand runs two or
three (e.g. calcium, alkalinity, magnesium) side by side, each with its own
bottle and schedule.

## Setting it up

1. Create a **GPIO switch** on the pin driving the pump's relay (see
   [your first device](/gekko/getting-started/first-device/) — it is the same
   flow).
2. Optionally create a **binary sensor** for the float switch.
3. Create the **dosing pump** device: pick the switch as its *pump switch*,
   the sensor as its *low-level sensor* (invertible per link), set the
   container capacity and warning threshold, and add dose slots to the
   schedule.

![Dosing pump settings in the portal](../../../../assets/screenshots/device-dosing-pump.png)

## Calibrate before trusting it

Gekko converts millilitres to seconds of run time through one number — the
pump's flow rate (`ml/s`). Tubing length, diameter, and head height all change
it, so measure it once with your real setup:

![Calibration: run a dose, measure the real volume, enter it](../../../../assets/diagrams/dosing-calibration.svg)

Calibration runs are excluded from statistics and history, but the dispensed
liquid **is** subtracted from the container — it really left the bottle. If you
already know the flow rate, a direct mode lets you type it in.

## Scheduling: how doses actually fire

The schedule holds dose slots (time of day + amount) and a day pattern — every
N days, or specific weekdays. The device evaluates it against its own clock, so
[give it a reliable time source](/gekko/reference/devices/schedule/#time--clock).
The daily total is split into several small doses on purpose — stability again.

![Dose timeline: on-time dose, dose within the 5-minute grace window, missed dose skipped](../../../../assets/diagrams/dosing-timeline.svg)

Two policies are worth understanding:

- **Grace window.** A slot may start up to 5 minutes late — for example if a
  manual dose or calibration was occupying the pump at the scheduled minute.
- **Drop, don't dose late.** A slot missed by more than the grace window is
  *skipped*, never deferred. After a reboot, a clock that syncs mid-day, or a
  long calibration, you will **not** get a burst of caught-up doses — for
  water chemistry, a late burst is worse than one missed micro-dose.

Also per slot: **skip next** suppresses exactly one upcoming occurrence (water
change day, vacation), and the **auto** toggle gates the whole scheduler while
manual doses keep working.

A dose, once started, runs entirely on the device — the portal only sends the
command. You can close the browser mid-dose; the firmware times the run,
switches the pump off, and books the dispensed amount. Only one run can be
active at a time (manual, scheduled, or calibration — none preempts another),
and anything that takes the device out of service force-stops the motor.

## Container tracking

Tell Gekko the bottle's capacity and it accounts for every millilitre:

![Container tracking: counter decreases, low-level alert, empty blocks auto dosing](../../../../assets/diagrams/dosing-container.svg)

- Below the **warning threshold** the portal raises an alert (bell + toast) —
  time to mix a new batch.
- **Empty** — counter at zero, or the float sensor tripped — raises a critical
  alert, and with **block auto dosing when empty** enabled, scheduled doses
  stop instead of running the pump dry.
- **`daysLeft`** projects how long the remaining volume lasts at the schedule's
  average daily consumption.
- After refilling, record it with the **Set volume** command.

## Dose journal

Every scheduled and manual dose is appended to an on-device journal — 90+ days
of history at typical dosing rates, stored on a dedicated flash partition so it
survives firmware and portal updates. The device page charts it;
`GET /api/dosejournal?deviceId=<id>&periodDays=<n>` serves it raw. The journal
is a fixed-size ring per pump: old records rotate away on their own, and one
pump's history can never crowd out another's.

## Commands (REST)

| Command | Payload | Effect |
| --- | --- | --- |
| `startDose` | `amountMl`, `logging` | Manual dose (`logging:false` = calibration run) |
| `stopDose` | — | Stop now, book the actual amount |
| `setVolume` | `volumeMl` | Container refill/correction |
| `skipNext` | `doseIndex`, `skip` | Skip one upcoming occurrence of a slot |
| `setMode` | `auto` / `manual` | Enable/disable the scheduler |

Full execution model and journal internals:
[`docs/dosing-pump.md`](https://github.com/yoreek/gekko/blob/master/docs/dosing-pump.md).
