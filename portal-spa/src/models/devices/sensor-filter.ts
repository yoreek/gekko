export interface SensorFilterConfig {
  smoothingWeight: number
  calibrationFactor: number
  calibrationOffset: number
}

// smoothingWeight=1 disables EMA smoothing (pass-through); calibrationFactor=1/calibrationOffset=0
// disables linear calibration -- matches the firmware's SensorReadingFilter no-op defaults.
export function defaultSensorFilterConfig(): SensorFilterConfig {
  return {
    smoothingWeight: 1,
    calibrationFactor: 1,
    calibrationOffset: 0,
  }
}

export function normalizeSensorFilterConfig(value: unknown, defaults: SensorFilterConfig = defaultSensorFilterConfig()): SensorFilterConfig {
  if (typeof value !== 'object' || value === null) {
    return { ...defaults }
  }
  const raw = value as Record<string, unknown>
  return {
    smoothingWeight: typeof raw.smoothingWeight === 'number' && Number.isFinite(raw.smoothingWeight)
      ? raw.smoothingWeight
      : defaults.smoothingWeight,
    calibrationFactor: typeof raw.calibrationFactor === 'number' && Number.isFinite(raw.calibrationFactor)
      ? raw.calibrationFactor
      : defaults.calibrationFactor,
    calibrationOffset: typeof raw.calibrationOffset === 'number' && Number.isFinite(raw.calibrationOffset)
      ? raw.calibrationOffset
      : defaults.calibrationOffset,
  }
}
