# Dosing Pump: Runs, Scheduling, and Calibration

How the `dosing_pump` device executes doses, why the web server never blocks on one, what
happens when two runs collide, and the fate of a dose that missed its slot. Firmware sources:
`src/devices/dosing/`; REST mapping: `src/portal/controllers/DeviceRegistryController.cpp` +
`src/integrations/rest/dosing_pump/`; SPA dialogs: `portal-spa/src/components/devices/dosing-pump/`.

## Execution model: fire-and-forget commands, backend-timed runs

The frontend only *issues commands*; it never times or completes a dose.

```
POST /api/devices/:id/command {command:"startDose", amountMl, logging}
  └─ controller composes the Custom-command grammar "startDose:<centiMl>:<0|1>"
       └─ DeviceRegistry::command() (registry mutex) → DosingPumpDevice::handleCommand()
            └─ startRun(): switch the pump relay ON, record runStartMs_/runEndMs_ — and RETURN
```

- The HTTP handler returns `{"success":true}` immediately; **no request ever waits for a run to
  finish**. The run duration is computed once up front: `runMs = amountCentiMl * 10000 /
  speedMilliMlPerSec` (`doseRunMs`).
- Completion is owned by the firmware tick: `tickReady()` checks `now >= runEndMs_` and calls
  `finishRun()`, which switches the relay OFF and books the **actually dosed** amount from the
  elapsed time. Closing the browser right after the command changes nothing.
- Progress is observed passively: while a run is active the runtime marks itself dirty every
  second, so WebSocket snapshots carry `dosedMl` / `dosingRemainingSec` for the UI progress bars.
- Safety: whenever the device leaves Ready (disable, delete, reconfigure, dependency loss),
  `abortRunIfActive()` finishes the run and force-switches the relay off — the motor can never be
  left running (`DosingPumpDevice.cpp`, `Ready` state guards + `end()`).

## Run types and collision rules

One `runActive_` flag serializes everything. There are three run types (`DosingRunType`):
`Scheduled`, `Manual` (`startDose` with `logging=true`), `Calibration` (`logging=false`).

- **A run in progress is never preempted.** `startRun()` rejects when `runActive_` is already
  set — regardless of who asks (manual over scheduled, calibration over manual, scheduled over
  calibration). The rejection propagates as `handleCommand() == false` →
  `400 {"success":false, code, error:"command rejected by runtime"}` — the "busy" answer.
  (The error code is the generic `InvalidCommand`; there is currently no dedicated BUSY code.)
- The scheduler itself only starts a dose when `!runActive_` (`evaluateSchedule`), so a schedule
  slot cannot interrupt a manual/calibration run either.
- The only way to cut a run short is an explicit `stopDose`; `finishRun()` then books the
  *actual* elapsed-time amount, not the target.

## Scheduled doses, the grace window, and missed doses

Schedule evaluation runs on the device tick (at most once per second) against the device-local
wall clock, and only when the clock is trusted (`year >= 2020`, otherwise `timeValid=false` and
nothing fires).

Each dose slot has a **grace window of `kDosingPumpGraceMinutes` (5 minutes)** starting at its
scheduled minute:

- Inside the window, an unfired slot starts (auto mode, device Ready, no active run). The fired
  bit is set **at start, and also on a failed start**, so a misbehaving pump switch cannot cause
  a retry storm.
- Past the window the slot is marked handled *without running* — the **drop-don't-dose-late**
  policy. A dose that could not run on time (long manual run or calibration occupying the pump,
  reboot, clock arriving mid-day, container refilled hours later) is **skipped, not deferred**:
  for aquarium chemistry a late burst of caught-up doses is worse than a missed one. It simply
  never lands in `todayDosedMl`. If a bounded catch-up is ever wanted, it belongs here in
  `evaluateSchedule`/`missedDosesMask` — keep it limited (e.g. only the most recent miss).

Related per-slot mechanics:

- `skipNext` suppresses exactly one upcoming occurrence of a slot, consumed when that occurrence
  arrives (or shown skipped in `nextDoseAt` projection).
- An empty container with `blockAutoWhenEmpty` marks due slots handled the same dropped-not-
  deferred way — no burst after a refill.
- `firedMask`/`todayDosedMl` reset on day change; a config update clears `firedMask` (slots may
  have been reordered) and lets past slots re-mark themselves as missed rather than re-dose.

## Calibration flow

Calibration measures the pump's real flow rate (`dosingSpeedMlPerSec`, stored in config as
milli-ml/s). The "by dose" mode in `CalibrationDialog.vue`:

1. `startDose` with `logging: false` → a `Calibration` run: the relay runs for
   `amountMl / currentSpeed` seconds, i.e. timed with the **old** speed.
2. The user measures the actually dispensed volume in a measuring cup and types it in.
3. The **frontend** computes `newSpeed = measuredMl / plannedRunSeconds`
   (`dosing-pump-math.ts`) and saves it via a normal `updateConfig` — there is no dedicated
   calibration endpoint or state on the backend.

The "direct" mode is step 3 alone. Calibration runs are excluded from `todayDosedMl`,
`lastDose`, and the dose journal (`finishRun` checks `runType_ != Calibration`), but the
dispensed volume **is** subtracted from the container — the liquid really left the bottle.

## Container accounting

- `containerCurrentCentiMl` is retained state (not config): decremented by every run's actual
  amount, set absolutely by the `setVolume` command (refill/correction), clamped to capacity.
- `container.status` in the runtime snapshot: `critical` when empty (level sensor active, or
  counter at 0), `warning` when `percent <= thresholdPercent` — this feeds the portal alert
  pipeline (see `docs/portal-alerts.md`).
- An optional level sensor (condition dependency, per-link invert) overrides the counter for
  emptiness detection; `daysLeft` projects the counter over the schedule's average daily volume.

## Dose journal: storage, capacity, retention

The journal is **not in NVS**. NVS holds only the pump's retained-state blob (one fixed-size
record per device: container volume, auto mode, fired/skip masks — see
`docs/device-registry-persistence.md`). History lives on the dedicated **`devdata` LittleFS
partition** (256 KiB, `my_partitions.csv`) as per-device append-only binary files:
`/dj/<deviceId>/seg0.bin` and `/dj/<deviceId>/seg1.bin` (`LittleFsDoseJournalStorage` mounts the
partition itself, formatting it on first boot). Appending an entry is a 12-byte `file.write`
append to the device's active file — no per-entry keys, no NVS blocks, no rewrite of existing
data.

**devdata naming convention:** one top-level directory per feature (the dose journal owns
`/dj`), one subdirectory per device id underneath. Device ids are registry-unique, so names
cannot collide; the feature directory is the type namespace. Future device-generated data
follows the same pattern under its own top-level directory.

**Record format** (`DoseJournalRecordV1`, packed, 12 bytes): `epoch` (u32, local-flavored
unixtime), `deviceId` (u32), `type` (u8: schedule/manual), `flags` (u8, reserved),
`amountCentiMl` (u16). All pumps share the one journal; reads filter by `deviceId`.

**Capacity — a two-segment ring per device** (`SegmentedDoseJournal`): each segment holds
`kDoseJournalSegmentBytes` = 16 KiB = **1365 records**. Appends fill the device's active
segment; when it is full the *other* segment is truncated and becomes active. So at least one
full segment of a device's history always survives a rotation, its on-disk size never exceeds
~32 KiB, and its record count oscillates between 1365 and 2730 — **90+ days at ~30 doses/day per
device**; the 256 KiB partition fits roughly 7 devices' journals. Rings are independent: a
chatty pump can never evict another pump's history. The active segment is derived from storage
on every use (the segment whose last record is newest), so there is no in-RAM journal state to
recover after a reboot.

**Write guarantees (best-effort by design):**

- Journal writes never gate dosing: a failed append just means no history record; run totals and
  retained state still update (`finishRun`).
- A free-space guard drops appends when the partition has less than `kDoseJournalMinFreeBytes`
  (8 KiB) free — LittleFS keeps spare blocks for wear-leveled writes.
- Calibration runs and runs finished with an invalid clock are not journaled.
- Deleting a device purges its `/dj/<deviceId>/` directory: `DoseJournalCleanupSink` listens for
  `DeviceDeleted` on the device-event dispatcher, so orphaned directories cannot accumulate.

**Reading:** `GET /api/dosejournal?deviceId=<id>&periodDays=<1-365>` (default 7 days) streams
entries newest-first straight from the segment files (32-record chunks, no heap buffering of the
whole journal). Omitting `deviceId` visits all devices sequentially (newest-first within each
device, not globally merged). With an untrusted clock the period cut is skipped and everything
the ring holds is returned.

**Why there is no retention-time setting:** the bound is structural (fixed-size ring), so old
records cost nothing — they are reclaimed 16 KiB at a time by segment rotation, and disk usage
cannot grow past the cap regardless of age. A time-based retention knob would add config and a
rewrite pass without saving space; the `periodDays` query parameter already limits what the UI
shows. If a hard age limit is ever genuinely needed, implement it as an epoch check during
segment rotation in `SegmentedDoseJournal::append`, not as per-record deletion.

**Why a dedicated partition:** the UI assets live on the separate `littlefs` partition, which
`pio run -t uploadfs` re-flashes as a **whole filesystem image** — anything not in `data/` would
be wiped. Keeping runtime-generated device data on its own `devdata` partition means UI updates
cannot touch the dose journal, and journal growth can never crowd out an asset update. The
partition was carved out of the app partition (which has ample headroom); if device data ever
outgrows it, the next step is external storage (SD card), not a bigger flash partition. `devdata`
is general-purpose: future device-generated files belong here too, namespaced by directory like
`/dj/`.

## Command reference (REST → device grammar)

| REST `command` | Payload fields | Device command | Notes |
|---|---|---|---|
| `startDose` | `amountMl` (0.01–655.35), `logging` (default true) | `startDose:<centiMl>:<0\|1>` | `logging=false` = calibration run |
| `stopDose` | — | `stopDose` | Always succeeds; books actual amount |
| `setVolume` | `volumeMl` >= 0 | `setVolume:<centiMl>` | Container refill/correction |
| `skipNext` | `doseIndex`, `skip` (default true) | `skipNext:<i>:<0\|1>` | One-shot per occurrence |
| `setMode` | `mode`: `auto`/`manual` | `auto` / `manual` | Auto gates the scheduler only |

All amounts travel as floats in ml over REST and as integer centi-ml (0.01 ml) inside the
firmware; speed is milli-ml/s. See `docs/rest-api-contract.md` for envelope conventions.
