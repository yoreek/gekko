// Pure dosing arithmetic shared by the dosing-pump dialogs and widget - ported from the
// DosingPumpFrontend reference (DosesGeneratorDialog/CalibrationDialog/ManualDoseDialog) and kept
// as plain functions so unit tests can exercise them without mounting components.

export interface GeneratedDose {
  time: string // "HH:mm"
  amountMl: number
}

export const DOSING_PUMP_MAX_DOSES = 16

export function minuteOfDayFromTime(time: string): number {
  const match = /^(\d{1,2}):(\d{2})$/.exec(time)
  if (!match) {
    return -1
  }
  const hours = Number(match[1])
  const minutes = Number(match[2])
  if (!Number.isFinite(hours) || !Number.isFinite(minutes) || hours > 23 || minutes > 59) {
    return -1
  }
  return hours * 60 + minutes
}

export function timeFromMinuteOfDay(minuteOfDay: number): string {
  const bounded = Math.min(Math.max(Math.round(minuteOfDay), 0), 24 * 60 - 1)
  const hours = Math.floor(bounded / 60)
  const minutes = bounded % 60
  return `${String(hours).padStart(2, '0')}:${String(minutes).padStart(2, '0')}`
}

function roundMl(value: number): number {
  return Math.round(value * 10) / 10
}

// Splits totalAmount into doseCount equally spaced doses between startTime and endTime (the last
// dose lands before endTime, not at it). Each dose is rounded to 0.1 ml; the last dose absorbs
// the rounding remainder so the sum matches totalAmount exactly. Mirrors the reference
// DosesGeneratorDialog::onGenerate, including its rejection rules.
export function generateDoses(startTime: string, endTime: string, totalAmount: number, doseCount: number): GeneratedDose[] | null {
  const startMinute = minuteOfDayFromTime(startTime)
  const endMinute = minuteOfDayFromTime(endTime)
  if (startMinute < 0 || endMinute < 0 || endMinute <= startMinute) {
    return null
  }
  if (!Number.isFinite(totalAmount) || totalAmount <= 0) {
    return null
  }
  if (!Number.isInteger(doseCount) || doseCount < 1 || doseCount > DOSING_PUMP_MAX_DOSES) {
    return null
  }
  const interval = (endMinute - startMinute) / doseCount
  if (interval < 1) {
    return null
  }
  const doseAmount = roundMl(totalAmount / doseCount)
  if (doseAmount <= 0) {
    return null
  }
  const doses: GeneratedDose[] = []
  let allocated = 0
  for (let index = 0; index < doseCount; index += 1) {
    const minute = startMinute + Math.round(index * interval)
    const isLast = index === doseCount - 1
    const amount = isLast ? roundMl(totalAmount - allocated) : doseAmount
    if (amount <= 0) {
      return null
    }
    allocated = roundMl(allocated + amount)
    doses.push({ time: timeFromMinuteOfDay(minute), amountMl: amount })
  }
  return doses
}

// amount / speed - how long the pump must run for the requested dose.
export function doseRunSeconds(amountMl: number, speedMlPerSec: number): number {
  if (!(speedMlPerSec > 0) || !(amountMl > 0)) {
    return 0
  }
  return amountMl / speedMlPerSec
}

// Calibration inverse: the new speed implied by the measured output of a timed test run.
export function calibrationSpeed(measuredMl: number, runSeconds: number): number {
  if (!(runSeconds > 0) || !(measuredMl > 0)) {
    return 0
  }
  return Math.round((measuredMl / runSeconds) * 1000) / 1000
}

// Speeds within 1% are treated as identical (reference SPEED_DIFF_THRESHOLD): re-saving a
// statistically indistinguishable calibration would just churn the config revision.
export function speedsAreEquivalent(currentSpeed: number, newSpeed: number): boolean {
  if (!(currentSpeed > 0)) {
    return newSpeed <= 0
  }
  return Math.abs(newSpeed - currentSpeed) / currentSpeed < 0.01
}

export function totalScheduleAmount(doses: { amountMl: number }[]): number {
  return roundMl(doses.reduce((sum, dose) => sum + (Number.isFinite(dose.amountMl) ? dose.amountMl : 0), 0))
}

// The browser's local days-since-1970, the anchorDay flavor the firmware's everyDays cycle uses
// (device-local unixtime / 86400). Sent when a daily schedule with everyDays > 1 is saved.
export function todayLocalDayNumber(now: Date = new Date()): number {
  const localEpochMs = now.getTime() - now.getTimezoneOffset() * 60000
  return Math.floor(localEpochMs / 86400000)
}

// Formats the firmware's local-flavored epoch seconds (device-local wall clock encoded as if it
// were UTC) for display: read the calendar fields back in the UTC time zone, never the local one.
export function formatLocalFlavoredEpoch(epochSeconds: number, localeTag = 'en-US'): string {
  if (!Number.isFinite(epochSeconds) || epochSeconds <= 0) {
    return ''
  }
  return new Intl.DateTimeFormat(localeTag, {
    month: 'short',
    day: 'numeric',
    hour: '2-digit',
    minute: '2-digit',
    hourCycle: 'h23',
    timeZone: 'UTC',
  }).format(new Date(epochSeconds * 1000))
}
