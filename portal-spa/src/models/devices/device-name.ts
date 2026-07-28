import type { DeviceTypeName } from '@/models/device-type-ids'

export const MAX_DEVICE_NAME_BYTES = 32

const utf8Encoder = new TextEncoder()

// Compact, locale-independent instance names. Full localized type names remain in the type
// picker; these values only seed the editable device-name field and deliberately leave room for
// the numeric suffix used when another device already has the same name.
export const DEFAULT_DEVICE_NAMES: Record<DeviceTypeName, string> = {
  dummy: 'Device',
  gpio_switch: 'GPIO switch',
  onewire_bus: 'OneWire',
  i2c_bus: 'I2C',
  spi_bus: 'SPI',
  ds18b20_temperature_sensor: 'DS18B20',
  ntc_thermistor_temperature_sensor: 'NTC',
  aht10: 'AHT10',
  dht11: 'DHT11',
  htu21: 'HTU21',
  thermostat: 'Thermostat',
  ssd1306: 'SSD1306',
  st7735: 'ST7735',
  rtc_ds3231: 'DS3231',
  rtc_ds1302: 'DS1302',
  pcf8574_expander: 'PCF8574',
  pcf8575_expander: 'PCF8575',
  port_expander_switch: 'Expander channel',
  schedule: 'Schedule',
  auto_switch: 'Auto switch',
  binary_sensor: 'Binary sensor',
  dosing_pump: 'Dosing pump',
  analog_output: 'Analog output',
  fade_analog_output: 'Fade output',
  scheduled_analog_output: 'Scheduled output',
  analog_output_composer: 'Output group',
  analog_port_input: 'Analog input',
  ads1115_hub: 'ADS1115',
  cd74hc4067_hub: 'CD74HC4067',
  analog_input_channel: 'Analog channel',
  lcd1602: 'LCD1602',
  lcd2004: 'LCD2004',
  tm1637: 'TM1637',
}

export function deviceNameByteLength(value: string): number {
  return utf8Encoder.encode(value.trim()).byteLength
}

export function isValidDeviceName(value: string): boolean {
  const trimmed = value.trim()
  return trimmed.length > 0 && deviceNameByteLength(trimmed) <= MAX_DEVICE_NAME_BYTES
}

function truncateToUtf8Bytes(value: string, maxBytes: number): string {
  let result = ''
  let byteLength = 0
  for (const character of value.trim()) {
    const characterBytes = utf8Encoder.encode(character).byteLength
    if (byteLength + characterBytes > maxBytes) {
      break
    }
    result += character
    byteLength += characterBytes
  }
  return result
}

function normalizedDeviceName(value: string): string {
  return value.trim().toLocaleLowerCase()
}

export function nextAvailableDeviceName(baseName: string, existingNames: Iterable<string>): string {
  const usedNames = new Set(Array.from(existingNames, normalizedDeviceName))
  let suffix = 1

  while (true) {
    const suffixText = suffix === 1 ? '' : ` ${suffix}`
    const availableBaseBytes = MAX_DEVICE_NAME_BYTES - utf8Encoder.encode(suffixText).byteLength
    const candidate = `${truncateToUtf8Bytes(baseName, availableBaseBytes)}${suffixText}`
    if (!usedNames.has(normalizedDeviceName(candidate))) {
      return candidate
    }
    suffix += 1
  }
}
