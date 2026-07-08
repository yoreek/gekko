import type { DeviceCreateDraftBase } from '@/models/devices/base'
import type { TemperatureOutputSnapshot, TemperatureUnit } from '@/api/contracts'
import type { DeviceRole } from '@/models/device-type-ids'
import { BaseDevice } from './base-device.ts'

// Shared base for device types that implement the temperature-reading runtime (DS18B20, NTC
// thermistor, ...). Holds the pieces that are genuinely identical across every such type;
// each concrete type still owns its own config shape and normalize/encode logic.
export abstract class TemperatureSensorDevice<
  TConfig extends object,
  TCreateDraft extends DeviceCreateDraftBase & object,
  TOutput extends object,
> extends BaseDevice<TConfig, TCreateDraft, TOutput> {
  readonly dependencyRole: DeviceRole = 'temperature_sensor'

  static readonly temperatureUnitOptions: TemperatureUnit[] = ['celsius', 'fahrenheit']

  static formatTemperature(output: TemperatureOutputSnapshot | undefined): string {
    if (!output?.valid) {
      return ''
    }
    return `${output.value.toFixed(2)} ${output.unitSymbol}`
  }
}
