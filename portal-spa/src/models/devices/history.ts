import type {
  DeviceOutputSnapshot,
  DeviceRecord,
  GpioSwitchOutputSnapshot,
  Ds18b20TemperatureSensorOutputSnapshot,
  Aht10SensorOutputSnapshot,
  Htu21SensorOutputSnapshot,
  NtcThermistorTemperatureSensorOutputSnapshot,
  ThermostatOutputSnapshot,
} from '@/api/contracts'

export type HistorySeriesKind = 'numeric' | 'binary'

export const kTemperatureSeries = 'temperature'
export const kHumiditySeries = 'humidity'
export const kSwitchStateSeries = 'switchState'

export interface HistorySeriesReading {
  key: string
  kind: HistorySeriesKind
  labelKey: string
  value: number | null
  unitSymbol?: string
}

function temperatureReading(
  snapshot:
    | Ds18b20TemperatureSensorOutputSnapshot
    | NtcThermistorTemperatureSensorOutputSnapshot
    | Aht10SensorOutputSnapshot
    | Htu21SensorOutputSnapshot
    | ThermostatOutputSnapshot
    | undefined,
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

function humidityReading(snapshot: Htu21SensorOutputSnapshot | undefined): HistorySeriesReading {
  const humidity = snapshot?.humidity
  return {
    key: kHumiditySeries,
    kind: 'numeric',
    labelKey: 'device.card.history.humidity',
    value: humidity?.valid ? humidity.value : null,
    unitSymbol: humidity?.unitSymbol,
  }
}

function humidityReadingFromAht10(snapshot: Aht10SensorOutputSnapshot | undefined): HistorySeriesReading {
  const humidity = snapshot?.humidity
  return {
    key: kHumiditySeries,
    kind: 'numeric',
    labelKey: 'device.card.history.humidity',
    value: humidity?.valid ? humidity.value : null,
    unitSymbol: humidity?.unitSymbol,
  }
}

function switchStateReading(state: boolean | undefined): HistorySeriesReading {
  return {
    key: kSwitchStateSeries,
    kind: 'binary',
    labelKey: 'device.card.history.switchState',
    value: typeof state === 'boolean' ? (state ? 1 : 0) : null,
  }
}

export function extractReadings(device: DeviceRecord): HistorySeriesReading[] {
  const output = (device.runtime as { output?: DeviceOutputSnapshot }).output

  switch (device.record.typeName) {
    case 'ds18b20_temperature_sensor':
      return [temperatureReading(output as Ds18b20TemperatureSensorOutputSnapshot | undefined)]
    case 'ntc_thermistor_temperature_sensor':
      return [temperatureReading(output as NtcThermistorTemperatureSensorOutputSnapshot | undefined)]
    case 'aht10': {
      const aht10Output = output as Aht10SensorOutputSnapshot | undefined
      return [temperatureReading(aht10Output), humidityReadingFromAht10(aht10Output)]
    }
    case 'htu21': {
      const htu21Output = output as Htu21SensorOutputSnapshot | undefined
      return [temperatureReading(htu21Output), humidityReading(htu21Output)]
    }
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
