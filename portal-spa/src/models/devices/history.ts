import type {
  DeviceOutputSnapshot,
  DeviceRecord,
  GpioSwitchOutputSnapshot,
  Ds18b20TemperatureSensorOutputSnapshot,
  NtcThermistorTemperatureSensorOutputSnapshot,
  ThermostatOutputSnapshot,
} from '@/api/contracts'

export type HistorySeriesKind = 'numeric' | 'binary'

export const kTemperatureSeries = 'temperature'
export const kSwitchStateSeries = 'switchState'

export interface HistorySeriesReading {
  key: string
  kind: HistorySeriesKind
  labelKey: string
  value: number | null
  unitSymbol?: string
}

function temperatureReading(
  snapshot: Ds18b20TemperatureSensorOutputSnapshot | NtcThermistorTemperatureSensorOutputSnapshot | ThermostatOutputSnapshot | undefined,
): HistorySeriesReading {
  const temperature = snapshot?.temperature
  return {
    key: kTemperatureSeries,
    kind: 'numeric',
    labelKey: 'device.card.history.temperature',
    value: temperature?.valid ? temperature.value : null,
    unitSymbol: temperature?.unitSymbol,
  }
}

function switchStateReading(state: 'off' | 'on' | 'disabled' | undefined): HistorySeriesReading {
  return {
    key: kSwitchStateSeries,
    kind: 'binary',
    labelKey: 'device.card.history.switchState',
    value: state === 'on' ? 1 : state === 'off' ? 0 : null,
  }
}

export function extractReadings(device: DeviceRecord): HistorySeriesReading[] {
  const output = (device.runtime as { output?: DeviceOutputSnapshot }).output

  switch (device.record.typeName) {
    case 'ds18b20_temperature_sensor':
      return [temperatureReading(output as Ds18b20TemperatureSensorOutputSnapshot | undefined)]
    case 'ntc_thermistor_temperature_sensor':
      return [temperatureReading(output as NtcThermistorTemperatureSensorOutputSnapshot | undefined)]
    case 'gpio_switch':
      return [switchStateReading((output as GpioSwitchOutputSnapshot | undefined)?.state)]
    case 'thermostat': {
      const thermostatOutput = output as ThermostatOutputSnapshot | undefined
      return [
        temperatureReading(thermostatOutput),
        switchStateReading(thermostatOutput?.actualSwitchState),
      ]
    }
    default:
      return []
  }
}
