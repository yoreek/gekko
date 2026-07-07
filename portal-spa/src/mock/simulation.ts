import type { TemperatureOutputSnapshot } from '@/api/contracts'
import { publishRealtimeMessage } from '@/realtime/bus'
import { loadMockDatabase, saveMockDatabase } from './database'
import { decorateDeviceRecord, publishThermostatDependents, refreshMockDerivedDeviceState } from './handlers'
import { normalizeFiniteNumber } from './device-factories'

const kSimTickMs = 1000
const kSimDriftRangeCelsius = 1.5
const kSimPullToCenter = 0.15

const sensorCenterCelsius = new Map<number, number>()

function resolutionStepCelsius(resolution: unknown): number {
  const bits = resolution === 9 || resolution === 10 || resolution === 11 ? resolution : 12
  return 0.0625 * 2 ** (12 - bits)
}

function roundToStep(value: number, step: number): number {
  return Math.round(value / step) * step
}

function nextDriftedValue(deviceId: number, currentValue: number, step: number): number {
  const center = sensorCenterCelsius.get(deviceId) ?? currentValue
  sensorCenterCelsius.set(deviceId, center)
  const pulled = currentValue + (center - currentValue) * kSimPullToCenter
  const noise = (Math.random() - 0.5) * 2 * Math.max(step, 0.05)
  const bounded = Math.min(center + kSimDriftRangeCelsius, Math.max(center - kSimDriftRangeCelsius, pulled + noise))
  return roundToStep(bounded, step)
}

export function simulateMockSensorDrift(now: number = Date.now()): void {
  const db = loadMockDatabase()
  const driftedSensorIds: number[] = []

  for (const device of db.devices) {
    const isTemperatureSensor = device.record.typeName === 'ds18b20_temperature_sensor'
      || device.record.typeName === 'ntc_thermistor_temperature_sensor'
    if (!isTemperatureSensor || !device.config.enabled) {
      continue
    }
    const temperature = (device.runtime.output as { temperature?: TemperatureOutputSnapshot } | undefined)?.temperature
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
    driftedSensorIds.push(device.record.id)
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

export function startMockSensorSimulation(): () => void {
  if (typeof window === 'undefined') {
    return () => {}
  }
  const timer = window.setInterval(() => simulateMockSensorDrift(), kSimTickMs)
  return () => window.clearInterval(timer)
}
