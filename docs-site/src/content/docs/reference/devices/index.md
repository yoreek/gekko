---
title: Device catalog
description: All 38 device types built into every Gekko firmware image.
sidebar:
  order: 1
  label: Device catalog
---

Every Gekko firmware image ships with all of these device types built in — you
create instances of them from the portal at runtime. Types marked with a link
have a dedicated reference page; the rest are documented here in brief, with
detailed pages coming iteratively.

## Buses & infrastructure

| Type | Purpose |
| --- | --- |
| [`onewire_bus`](/gekko/reference/devices/onewire-bus/) | 1-Wire bus on a GPIO; parent for DS18B20 probes, with device scan |
| [`i2c_bus`](/gekko/reference/devices/i2c-bus/) | I2C bus (SDA/SCL); parent for displays, HTU21, RTC, port expanders |
| [`spi_bus`](/gekko/reference/devices/spi-bus/) | SPI bus; parent for the ST7735 TFT |
| `rtc_ds3231` | DS3231 real-time clock — keeps schedules running without NTP |
| `rtc_ds1302` | DS1302 three-wire real-time clock on CLK, DAT, and RST GPIO pins |
| `dummy` | Placeholder/test device |

## Switches & outputs

| Type | Purpose |
| --- | --- |
| [`gpio_switch`](/gekko/reference/devices/gpio-switch/) | On/off output on a GPIO — relays, MOSFETs, LEDs |
| [`pcf8574_expander`](/gekko/reference/devices/port-expanders/) / [`pcf8575_expander`](/gekko/reference/devices/port-expanders/) | 8-/16-bit I2C port expanders |
| [`port_expander_switch`](/gekko/reference/devices/port-expanders/) | A switch on one expander pin — same options as a GPIO switch |
| [`analog_output`](/gekko/reference/devices/analog-outputs/) | LEDC PWM output channel (dimmable light, fan, …) |
| [`fade_analog_output`](/gekko/reference/devices/analog-outputs/) | Smooth fade transitions for an analog output |
| [`scheduled_analog_output`](/gekko/reference/devices/analog-outputs/) | Daily level curve driving an analog output |
| [`analog_output_composer`](/gekko/reference/devices/analog-outputs/) | Groups analog channels into one fixture — see the [aquarium light example](/gekko/reference/devices/analog-outputs/#worked-example-a-five-channel-aquarium-light) |

## Lighting effects

| Type | Purpose |
| --- | --- |
| [`pixel_strip`](/gekko/reference/devices/pixel-strip/) | WS2812B addressable RGB strip on one GPIO — Adafruit NeoPixel hardware backend |
| [`pixel_effect_solid`](/gekko/reference/devices/pixel-strip/) | Fills a target `pixel_strip` with one static color |
| [`pixel_effect_alert`](/gekko/reference/devices/pixel-strip/) | Blinks a target `pixel_strip` while ANDed conditions are satisfied — e.g. an overflow/alarm indicator |

## Analog inputs

| Type | Purpose |
| --- | --- |
| [`analog_port_input`](/gekko/reference/devices/analog-inputs/) | A voltage reading straight off one ESP32 ADC pin, no extra hardware |
| [`ads1115_hub`](/gekko/reference/devices/analog-inputs/) | ADS1115 16-bit I2C ADC — 4 precise channels |
| [`cd74hc4067_hub`](/gekko/reference/devices/analog-inputs/) | CD74HC4067 16-channel analog multiplexer on one ADC pin |
| [`analog_input_channel`](/gekko/reference/devices/analog-inputs/) | One channel of an ADS1115 or CD74HC4067 hub |

## Sensors

| Type | Purpose |
| --- | --- |
| [`ds18b20_temperature_sensor`](/gekko/reference/devices/ds18b20/) | DS18B20 1-Wire temperature probe |
| [`ntc_thermistor_temperature_sensor`](/gekko/reference/devices/ntc-thermistor/) | NTC thermistor on any analog input |
| [`htu21`](/gekko/reference/devices/htu21/) | HTU21 I2C temperature + humidity sensor |
| `aht10` | AHT10 I2C temperature + humidity sensor |
| `dht11` | DHT11 temperature + humidity sensor on one GPIO |
| `binary_sensor` | Digital input — float switch, door contact, leak sensor |

## Control & automation

| Type | Purpose |
| --- | --- |
| [`thermostat`](/gekko/reference/devices/thermostat/) | Hysteresis heat/cool control: sensor in, switch out |
| [`schedule`](/gekko/reference/devices/schedule/) | Minute-precision time-of-day/weekday rules |
| `auto_switch` | Drives a switch from ANDed conditions, with override and pause — see [Schedules & automation](/gekko/guides/schedules-and-automation/) |
| [`dosing_pump`](/gekko/reference/devices/dosing-pump/) | Dosing runs with calibration, scheduling, and a dose journal |

## Displays

| Type | Purpose |
| --- | --- |
| `ssd1306` | I2C OLED with the [visual layout designer](/gekko/guides/displays/) |
| `st7735` | SPI color TFT, same layout designer |
| `lcd1602` | HD44780 16 × 2 character LCD through an embedded PCF8574 I2C backpack |
| `lcd2004` | HD44780 20 × 4 character LCD through an embedded PCF8574 I2C backpack |
| `lcd1602_pin` | HD44780 16 × 2 character LCD wired directly to ESP32 GPIO pins, see the [displays guide](/gekko/guides/displays/) |
| `lcd2004_pin` | HD44780 20 × 4 character LCD wired directly to ESP32 GPIO pins, see the [displays guide](/gekko/guides/displays/) |
| `tm1637` | Four-digit seven-segment display with brightness and 180° rotation |
