// Pure calibration arithmetic for the linear sensor filter (value = raw * factor + offset).
// Kept as plain functions so unit tests can exercise them without mounting the dialog.
//
// The runtime only exposes the *filtered* reading (already scaled + offset + smoothed), not the
// raw sensor value. So calibration works by inverting the currently-applied coefficients to
// recover the raw reading behind a displayed value, then solving new coefficients from the
// user's reference points. At steady state the smoothing term is a no-op, so inverting the
// displayed value is exact - hence the dialog asks the user to calibrate on a stable reading.

export interface FilterCoefficients {
  calibrationFactor: number
  calibrationOffset: number
}

export interface CalibrationPoint {
  displayed: number // the value the sensor currently shows (post-filter)
  real: number // the reference/true value at that moment
}

// Recover the raw sensor reading behind a displayed (calibrated) value.
export function rawFromDisplayed(displayed: number, current: FilterCoefficients): number {
  return (displayed - current.calibrationOffset) / current.calibrationFactor
}

// One reference point: keep the current factor, shift only the offset so `displayed` reads `real`.
// newOffset = real - raw*factor = real - (displayed - oldOffset) = oldOffset + (real - displayed).
export function solveOffsetOnly(point: CalibrationPoint, current: FilterCoefficients): FilterCoefficients {
  return {
    calibrationFactor: current.calibrationFactor,
    calibrationOffset: current.calibrationOffset + (point.real - point.displayed),
  }
}

// Two reference points: solve both slope and offset against the recovered raw readings.
// Returns null when the points are degenerate (equal raw readings, or non-finite results).
export function solveTwoPoint(p1: CalibrationPoint, p2: CalibrationPoint, current: FilterCoefficients): FilterCoefficients | null {
  const raw1 = rawFromDisplayed(p1.displayed, current)
  const raw2 = rawFromDisplayed(p2.displayed, current)
  const rawSpan = raw2 - raw1
  if (!Number.isFinite(rawSpan) || rawSpan === 0) {
    return null
  }
  const calibrationFactor = (p2.real - p1.real) / rawSpan
  const calibrationOffset = p1.real - calibrationFactor * raw1
  if (!Number.isFinite(calibrationFactor) || !Number.isFinite(calibrationOffset) || calibrationFactor === 0) {
    return null
  }
  return { calibrationFactor, calibrationOffset }
}

// Round coefficients to the precision the firmware stores (float) and the UI edits, so the
// computed values match what a user would hand-enter and don't churn with float noise.
export function roundCoefficients(coefficients: FilterCoefficients): FilterCoefficients {
  return {
    calibrationFactor: Math.round(coefficients.calibrationFactor * 1e6) / 1e6,
    calibrationOffset: Math.round(coefficients.calibrationOffset * 1e6) / 1e6,
  }
}
