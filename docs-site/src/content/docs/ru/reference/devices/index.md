---
title: Каталог устройств
description: Все 27 типов устройств, встроенных в каждый образ прошивки Gekko.
sidebar:
  order: 1
  label: Каталог устройств
---

В каждый образ прошивки Gekko уже встроены все эти типы устройств — вы просто
создаёте их экземпляры из портала во время работы. Типы с ссылками имеют
отдельные справочные страницы; остальные кратко описаны здесь, а подробные
страницы будут добавляться постепенно.

## Шины и инфраструктура

| Тип | Назначение |
| --- | --- |
| [`onewire_bus`](/gekko/ru/reference/devices/onewire-bus/) | Шина 1-Wire на GPIO; родитель для датчиков DS18B20, со сканированием устройств |
| [`i2c_bus`](/gekko/ru/reference/devices/i2c-bus/) | Шина I2C (SDA/SCL); родитель для дисплеев, HTU21, RTC, port expanders |
| [`spi_bus`](/gekko/ru/reference/devices/spi-bus/) | Шина SPI; родитель для TFT ST7735 |
| `rtc_ds3231` | Часы реального времени DS3231 — поддерживают расписания без NTP |
| `dummy` | Заглушка/тестовое устройство |

## Выключатели и выходы

| Тип | Назначение |
| --- | --- |
| [`gpio_switch`](/gekko/ru/reference/devices/gpio-switch/) | Выход on/off на GPIO — реле, MOSFET, светодиоды |
| [`pcf8574_expander`](/gekko/ru/reference/devices/port-expanders/) / [`pcf8575_expander`](/gekko/ru/reference/devices/port-expanders/) | 8-/16-битные I2C-расширители портов |
| [`port_expander_switch`](/gekko/ru/reference/devices/port-expanders/) | Выключатель на одном пине расширителя — те же параметры, что у GPIO switch |
| [`analog_output`](/gekko/ru/reference/devices/analog-outputs/) | Канал PWM на LEDC (диммируемый свет, вентилятор, …) |
| [`fade_analog_output`](/gekko/ru/reference/devices/analog-outputs/) | Плавные fade-переходы для analog output |
| [`scheduled_analog_output`](/gekko/ru/reference/devices/analog-outputs/) | Суточная кривая уровня, управляющая analog output |
| [`analog_output_composer`](/gekko/ru/reference/devices/analog-outputs/) | Группирует аналоговые каналы в один светильник — см. [пример аквариумного света](/gekko/ru/reference/devices/analog-outputs/#пример-пятиканальный-аквариумный-свет) |

## Аналоговые входы

| Тип | Назначение |
| --- | --- |
| [`analog_port_input`](/gekko/ru/reference/devices/analog-inputs/) | Измерение напряжения прямо с одного ADC-пина ESP32, без дополнительного железа |
| [`ads1115_hub`](/gekko/ru/reference/devices/analog-inputs/) | ADS1115 16-битный I2C ADC — 4 точных канала |
| [`cd74hc4067_hub`](/gekko/ru/reference/devices/analog-inputs/) | CD74HC4067 16-канальный аналоговый мультиплексор на одном ADC-пине |
| [`analog_input_channel`](/gekko/ru/reference/devices/analog-inputs/) | Один канал хаба ADS1115 или CD74HC4067 |

## Датчики

| Тип | Назначение |
| --- | --- |
| [`ds18b20_temperature_sensor`](/gekko/ru/reference/devices/ds18b20/) | Температурный датчик DS18B20 1-Wire |
| [`ntc_thermistor_temperature_sensor`](/gekko/ru/reference/devices/ntc-thermistor/) | NTC-термистор на любом аналоговом входе |
| [`htu21`](/gekko/ru/reference/devices/htu21/) | I2C-датчик температуры и влажности HTU21 |
| `binary_sensor` | Цифровой вход — поплавок, дверной контакт, датчик протечки |

## Управление и автоматика

| Тип | Назначение |
| --- | --- |
| [`thermostat`](/gekko/ru/reference/devices/thermostat/) | Управление нагревом/охлаждением с гистерезисом: датчик на входе, выключатель на выходе |
| [`schedule`](/gekko/ru/reference/devices/schedule/) | Правила времени суток/дней недели с точностью до минуты |
| `auto_switch` | Управляет выключателем по AND-условиям, с override и паузой — см. [Schedules & automation](/gekko/ru/guides/schedules-and-automation/) |
| [`dosing_pump`](/gekko/ru/reference/devices/dosing-pump/) | Дозирования с калибровкой, расписанием и журналом доз |

## Дисплеи

| Тип | Назначение |
| --- | --- |
| `ssd1306` | I2C OLED с [визуальным редактором раскладки](/gekko/ru/guides/displays/) |
| `st7735` | Цветной SPI TFT, тот же редактор раскладки |
