import type { AnalogInputReadingSnapshot, HumidityOutputSnapshot, TemperatureOutputSnapshot } from '@/api/contracts'
import { publishRealtimeMessage } from '@/realtime/bus'
import { loadMockDatabase, saveMockDatabase } from './database'
import { decorateDeviceRecord, publishThermostatDependents, refreshMockDerivedDeviceState, tickMockDosingRuns } from './handlers'
import { normalizeFiniteNumber } from './device-factories'

const kSimTickMs = 1000
const kSimDriftRangeCelsius = 1.5
const kSimPullToCenter = 0.15
const kSimDriftRangeMilliVolts = 60

const sensorCenterCelsius = new Map<number, number>()
const analogInputCenterMilliVolts = new Map<number, number>()

const ANALOG_INPUT_LEAF_TYPE_NAMES = new Set(['analog_port_input', 'analog_input_channel'])

function nextDriftedGeneric(centerMap: Map<number, number>, deviceId: number, currentValue: number, step: number, range: number): number {
  const center = centerMap.get(deviceId) ?? currentValue
  centerMap.set(deviceId, center)
  const pulled = currentValue + (center - currentValue) * kSimPullToCenter
  const noise = (Math.random() - 0.5) * 2 * Math.max(step, 0.05)
  const bounded = Math.min(center + range, Math.max(center - range, pulled + noise))
  return roundToStep(bounded, step)
}

function resolutionStepCelsius(resolution: unknown): number {
  const bits = resolution === 9 || resolution === 10 || resolution === 11 ? resolution : 12
  return 0.0625 * 2 ** (12 - bits)
}

function roundToStep(value: number, step: number): number {
  return Math.round(value / step) * step
}

function nextDriftedValue(deviceId: number, currentValue: number, step: number): number {
  return nextDriftedGeneric(sensorCenterCelsius, deviceId, currentValue, step, kSimDriftRangeCelsius)
}

export function simulateMockSensorDrift(now: number = Date.now()): void {
  const db = loadMockDatabase()
  const driftedSensorIds: number[] = []

  for (const device of db.devices) {
    if (!device.config.enabled) {
      continue
    }
    const isTemperatureSensor = device.record.typeName === 'ds18b20_temperature_sensor'
      || device.record.typeName === 'ntc_thermistor_temperature_sensor'
      || device.record.typeName === 'aht10'
      || device.record.typeName === 'htu21'
    if (isTemperatureSensor) {
      const output = device.runtime.output as { temperature?: TemperatureOutputSnapshot; humidity?: HumidityOutputSnapshot } | undefined
      const temperature = output?.temperature
      if (!temperature || !temperature.valid) {
        continue
      }
      const pollMs = Math.max(1000, normalizeFiniteNumber(device.config.pollMs, 5000))
      if (now - temperature.measuredAtMs < pollMs) {
        continue
      }

      const step = device.record.typeName === 'ds18b20_temperature_sensor' ? resolutionStepCelsius(device.config.resolution) : 0.1
      device.runtime.output = {
        ...(device.runtime.output as Record<string, unknown>),
        temperature: {
          ...temperature,
          value: nextDriftedValue(device.record.id, temperature.value, step),
          measuredAtMs: now,
        },
      }
      const humidity = output?.humidity
      if ((device.record.typeName === 'htu21' || device.record.typeName === 'aht10') && humidity?.valid) {
        // Reuse the temperature drift model with a separate map key so the two channels wander
        // independently; humidity stays clamped to its physical range.
        const drifted = nextDriftedValue(-device.record.id, humidity.value, 0.1)
        ;(device.runtime.output as { humidity?: HumidityOutputSnapshot }).humidity = {
          ...humidity,
          value: Math.min(100, Math.max(0, drifted)),
          measuredAtMs: now,
        }
      }
      driftedSensorIds.push(device.record.id)
      continue
    }

    if (ANALOG_INPUT_LEAF_TYPE_NAMES.has(device.record.typeName)) {
      const output = device.runtime.output as { analogInput?: AnalogInputReadingSnapshot } | undefined
      const reading = output?.analogInput
      if (!reading || !reading.valid) {
        continue
      }
      const pollMs = Math.max(100, normalizeFiniteNumber(device.config.pollMs, 1000))
      if (now - (reading.measuredAtMs ?? 0) < pollMs) {
        continue
      }

      const nextMilliVolts = Math.max(0, Math.min(3300, nextDriftedGeneric(
        analogInputCenterMilliVolts, device.record.id, reading.milliVolts ?? 0, 5, kSimDriftRangeMilliVolts,
      )))
      device.runtime.output = {
        ...(device.runtime.output as Record<string, unknown>),
        analogInput: {
          ...reading,
          milliVolts: nextMilliVolts,
          rawCode: Math.max(0, Math.min(4095, Math.round((nextMilliVolts / 3300) * 4095))),
          measuredAtMs: now,
        },
      }
      driftedSensorIds.push(device.record.id)
    }
  }

  if (driftedSensorIds.length === 0) {
    return
  }

  refreshMockDerivedDeviceState(db)
  saveMockDatabase(db)

  for (const deviceId of driftedSensorIds) {
    const device = db.devices.find(entry => entry.record.id === deviceId)
    if (!device) {
      continue
    }
    publishRealtimeMessage({
      topic: 'device.upsert',
      revision: db.registryRevision,
      payload: {
        ...decorateDeviceRecord(device, db.registryRevision),
        eventKind: 'device_updated',
      },
    })
    publishThermostatDependents(db, deviceId)
  }
}

// Live dosing progress: while a mock dose run is active its dosedMl/remaining-seconds countdown
// updates once per tick and the completed run lands in the container/journal - the mock stand-in
// for DosingPumpDevice's markRuntimeStateDirty()-per-second WS pushes.
export function simulateMockDosing(now: number = Date.now()): void {
  const db = loadMockDatabase()
  const updatedIds = tickMockDosingRuns(db, now)
  if (updatedIds.length === 0) {
    return
  }
  saveMockDatabase(db)
  for (const deviceId of updatedIds) {
    const device = db.devices.find(entry => entry.record.id === deviceId)
    if (!device) {
      continue
    }
    publishRealtimeMessage({
      topic: 'device.upsert',
      revision: db.registryRevision,
      payload: {
        ...decorateDeviceRecord(device, db.registryRevision),
        eventKind: 'device_updated',
      },
    })
  }
}

export function startMockSensorSimulation(): () => void {
  if (typeof window === 'undefined') {
    return () => {}
  }
  const timer = window.setInterval(() => {
    simulateMockSensorDrift()
    simulateMockDosing()
  }, kSimTickMs)
  return () => window.clearInterval(timer)
}
