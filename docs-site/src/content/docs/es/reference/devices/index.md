---
title: Catálogo de dispositivos
description: Los 33 tipos de dispositivo incorporados en cada imagen de firmware de Gekko.
sidebar:
  order: 1
  label: Catálogo de dispositivos
---

Cada imagen de firmware de Gekko trae ya todos estos tipos de dispositivo: tú
creas instancias de ellos desde el portal en tiempo de ejecución. Los tipos
marcados con enlace tienen una página de referencia dedicada; los demás se
documentan aquí de forma breve, con páginas detalladas que irán apareciendo
iterativamente.

## Buses e infraestructura

| Tipo | Propósito |
| --- | --- |
| [`onewire_bus`](/gekko/es/reference/devices/onewire-bus/) | Bus 1-Wire en un GPIO; padre de sondas DS18B20, con escaneo de dispositivos |
| [`i2c_bus`](/gekko/es/reference/devices/i2c-bus/) | Bus I2C (SDA/SCL); padre de pantallas, HTU21, RTC, expansores de puertos |
| [`spi_bus`](/gekko/es/reference/devices/spi-bus/) | Bus SPI; padre del TFT ST7735 |
| `rtc_ds3231` | Reloj en tiempo real DS3231 - mantiene los horarios funcionando sin NTP |
| `rtc_ds1302` | Reloj en tiempo real DS1302 de tres hilos en GPIO CLK, DAT y RST |
| `dummy` | Dispositivo marcador de posición/pruebas |

## Interruptores y salidas

| Tipo | Propósito |
| --- | --- |
| [`gpio_switch`](/gekko/es/reference/devices/gpio-switch/) | Salida on/off en un GPIO - relés, MOSFETs, LEDs |
| [`pcf8574_expander`](/gekko/es/reference/devices/port-expanders/) / [`pcf8575_expander`](/gekko/es/reference/devices/port-expanders/) | Expansores de puertos I2C de 8/16 bits |
| [`port_expander_switch`](/gekko/es/reference/devices/port-expanders/) | Un interruptor sobre un pin del expansor - mismas opciones que un GPIO switch |
| [`analog_output`](/gekko/es/reference/devices/analog-outputs/) | Canal PWM LEDC (luz regulable, ventilador, ...) |
| [`fade_analog_output`](/gekko/es/reference/devices/analog-outputs/) | Transiciones suaves para una salida analógica |
| [`scheduled_analog_output`](/gekko/es/reference/devices/analog-outputs/) | Curva diaria de nivel que gobierna una salida analógica |
| [`analog_output_composer`](/gekko/es/reference/devices/analog-outputs/) | Agrupa varios canales analógicos en una sola luminaria - ver el [ejemplo de luz de acuario](/gekko/es/reference/devices/analog-outputs/#ejemplo-completo-una-luz-de-acuario-de-cinco-canales) |

## Entradas analógicas

| Tipo | Propósito |
| --- | --- |
| [`analog_port_input`](/gekko/es/reference/devices/analog-inputs/) | Una lectura de voltaje directa desde un pin ADC del ESP32, sin hardware extra |
| [`ads1115_hub`](/gekko/es/reference/devices/analog-inputs/) | ADC I2C ADS1115 de 16 bits - 4 canales precisos |
| [`cd74hc4067_hub`](/gekko/es/reference/devices/analog-inputs/) | Multiplexor analógico CD74HC4067 de 16 canales sobre un pin ADC |
| [`analog_input_channel`](/gekko/es/reference/devices/analog-inputs/) | Un canal de un hub ADS1115 o CD74HC4067 |

## Sensores

| Tipo | Propósito |
| --- | --- |
| [`ds18b20_temperature_sensor`](/gekko/es/reference/devices/ds18b20/) | Sonda de temperatura 1-Wire DS18B20 |
| [`ntc_thermistor_temperature_sensor`](/gekko/es/reference/devices/ntc-thermistor/) | Termistor NTC sobre cualquier entrada analógica |
| [`htu21`](/gekko/es/reference/devices/htu21/) | Sensor I2C HTU21 de temperatura + humedad |
| `aht10` | Sensor I2C AHT10 de temperatura + humedad |
| `dht11` | Sensor DHT11 de temperatura + humedad en un GPIO |
| `binary_sensor` | Entrada digital - flotador, contacto de puerta, sensor de fuga |

## Control y automatización

| Tipo | Propósito |
| --- | --- |
| [`thermostat`](/gekko/es/reference/devices/thermostat/) | Control de calefacción/refrigeración con histéresis: sensor entra, interruptor sale |
| [`schedule`](/gekko/es/reference/devices/schedule/) | Reglas de hora del día y día de la semana con precisión de minuto |
| `auto_switch` | Gobierna un interruptor a partir de condiciones ANDed, con override y pausa - ver [Schedules y automatización](/gekko/es/guides/schedules-and-automation/) |
| [`dosing_pump`](/gekko/es/reference/devices/dosing-pump/) | Dosis programadas con calibración, contabilidad del depósito y diario de dosis |

## Pantallas

| Tipo | Propósito |
| --- | --- |
| `ssd1306` | OLED I2C con el [diseñador visual de disposición](/gekko/es/guides/displays/) |
| `st7735` | TFT a color SPI, el mismo diseñador de disposición |
| `lcd1602` | LCD de caracteres HD44780 de 16 × 2 mediante expansor PCF857x |
| `lcd2004` | LCD de caracteres HD44780 de 20 × 4 mediante expansor PCF857x |
| `tm1637` | Pantalla de siete segmentos de cuatro dígitos con brillo y giro de 180° |
