---
title: Device catalog
description: All 23 device types built into every Gekko firmware image.
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
| `i2c_bus` | I2C bus (SDA/SCL); parent for displays, HTU21, RTC, port expanders |
| `spi_bus` | SPI bus; parent for the ST7735 TFT |
| `rtc_ds3231` | DS3231 real-time clock — keeps schedules running without NTP |
| `dummy` | Placeholder/test device |

## Switches & outputs

| Type | Purpose |
| --- | --- |
| [`gpio_switch`](/gekko/reference/devices/gpio-switch/) | On/off output on a GPIO — relays, MOSFETs, LEDs |
| `pcf8574_expander` / `pcf8575_expander` | 8-/16-bit I2C port expanders |
| `port_expander_switch` | A switch on one expander pin — same options as a GPIO switch |
| [`analog_output`](/gekko/reference/devices/analog-outputs/) | LEDC PWM output channel (dimmable light, fan, …) |
| [`fade_analog_output`](/gekko/reference/devices/analog-outputs/) | Smooth fade transitions for an analog output |
| [`scheduled_analog_output`](/gekko/reference/devices/analog-outputs/) | Daily level curve driving an analog output |
| [`analog_output_composer`](/gekko/reference/devices/analog-outputs/) | Groups analog channels into one fixture — see the [aquarium light example](/gekko/reference/devices/analog-outputs/#worked-example-an-aquarium-light) |

## Sensors

| Type | Purpose |
| --- | --- |
| [`ds18b20_temperature_sensor`](/gekko/reference/devices/ds18b20/) | DS18B20 1-Wire temperature probe |
| `ntc_thermistor_temperature_sensor` | NTC thermistor on an ADC pin |
| `htu21` | HTU21 I2C temperature + humidity sensor |
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
