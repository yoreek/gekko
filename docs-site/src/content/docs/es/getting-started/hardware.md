---
title: Requisitos de hardware
description: Lo que necesitas para ejecutar Gekko - una placa de desarrollo ESP32 sencilla con 4 MB de flash.
sidebar:
  order: 2
---

## La placa controladora

Gekko apunta al **ESP32** clásico (el chip original de doble núcleo) con
**4 MB de flash** - la típica placa de desarrollo "ESP32 DevKit" que suele
costar unos pocos dólares. Esa es la configuración para la que se construyen
los binarios precompilados:

| Partición | Offset de flash |
| --- | --- |
| Bootloader | `0x1000` |
| Tabla de particiones | `0x8000` |
| Firmware (una sola app, sin OTA) | `0x10000` |
| LittleFS (assets del portal web) | `0x370000` |

La compilación por defecto usa un diseño de una sola app sin ranura OTA para
que quepan el firmware, el portal web y la configuración del dispositivo en
4 MB. Las placas con más flash también funcionan y dejan margen para la
[compilación Web OTA](/gekko/es/guides/ota-updates/) opcional.

También necesitarás un **cable USB de datos** y, en algunas placas, el
controlador USB-serie habitual CP210x/CH340 para tu sistema operativo.

## Periféricos del catálogo de dispositivos

Todo lo siguiente es opcional - lo añades desde el portal web cuando
realmente lo cableas:

- **Relés / placas MOSFET** en cualquier GPIO libre (`gpio_switch`)
- **PCF8574 / PCF8575** expansores I2C para más salidas de conmutación
- **DS18B20** sondas de temperatura impermeables en un bus 1-Wire (un GPIO,
  muchas sondas)
- **Termistores NTC** y otros sensores analógicos en un pin ADC, un ADC
  I2C **ADS1115** de 16 bits o un multiplexor **CD74HC4067** de 16 canales
- **HTU21** sensor I2C de temperatura + humedad
- **SSD1306** pantallas OLED I2C y **ST7735** pantallas TFT SPI
- **DS3231** reloj en tiempo real I2C - recomendado si usas horarios y el
  dispositivo puede funcionar sin Internet/NTP
- **Bombas peristálticas dosificadoras** accionadas a través de una salida de
  conmutación
- **Controladores LED / cargas PWM** en pines compatibles con LEDC
  (`analog_output`)
- Entradas digitales: flotadores, contactos de puerta, sensores de fuga
  (`binary_sensor`)

Consulta el [catálogo de dispositivos](/gekko/es/reference/devices/) para la
lista completa de los 27 tipos incorporados.

:::tip
Empieza solo con la placa desnuda. Flashea, conéctala a WiFi y explora el
portal: podrás añadir hardware real más adelante, uno por uno.
:::
