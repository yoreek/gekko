---
title: Bus SPI
description: "Cómo funciona el bus SPI: líneas compartidas de reloj y datos, chip-select por dispositivo y el control del TFT ST7735 en Gekko."
sidebar:
  order: 5
  label: Bus SPI
---

## ¿Qué es SPI?

SPI (Serial Peripheral Interface) es el bus serie rápido: donde
[I2C](/gekko/es/reference/devices/i2c-bus/) llega a 400 kHz, SPI funciona con
alegría a decenas de MHz, por eso las pantallas en color lo usan. La
contrapartida es más cableado: un **reloj** compartido (SCK) y **datos**
(MOSI, opcionalmente MISO para datos de vuelta), más una línea individual de
**chip-select** (CS) por dispositivo: así se distinguen los dispositivos en
lugar de usar direcciones.

En Gekko el bus es el dispositivo `spi_bus`: posee los pines compartidos SCK/
MOSI/MISO y el host SPI del ESP32. Hoy su único consumidor es la pantalla
**TFT en color ST7735**; los pines CS/DC/reset de la pantalla pertenecen al
dispositivo de pantalla, no al bus.

## Cableado

![Cableado SPI: SCK y MOSI compartidos hacia el ST7735, chip-select y pines de datos/comando por pantalla](../../../../../assets/diagrams/spi-wiring.svg)

Un módulo ST7735 típico se conecta así (los nombres impresos pueden variar):

| Pin del módulo | Pin del ESP32 (por defecto) | Pertenece a |
| --- | --- | --- |
| SCK / CLK | GPIO 18 | `spi_bus` |
| SDA / MOSI / DIN | GPIO 23 | `spi_bus` |
| CS | GPIO 5 | dispositivo `st7735` |
| DC / A0 | GPIO 2 | dispositivo `st7735` |
| RES / RST | — (o cualquier GPIO libre) | dispositivo `st7735` |
| VCC, GND, LED/BLK | 3,3 V / GND | alimentación |

Las pantallas nunca devuelven datos, así que **MISO se queda en -1** (sin
uso). Un segundo dispositivo SPI compartiría SCK/MOSI y simplemente tendría
su propio pin CS.

## Dispositivo de bus y diagnóstico

Como los otros buses, `spi_bus` muestra diagnósticos en vivo: contadores de
errores y estado de transacción, y deshabilitarlo bloquea la pantalla con
`dependency_blocked` en vez de dejar píxeles antiguos.

![Ajustes del bus SPI con diagnósticos](../../../../../assets/screenshots/device-spi-bus.png)

## Configuración

| Campo | Valor por defecto | Significado |
| --- | --- | --- |
| `host` | `VSPI` | Qué controlador SPI de hardware del ESP32 usar |
| `sckPin` | `18` | Reloj compartido |
| `mosiPin` | `23` | Datos de salida compartidos |
| `misoPin` | `-1` | Datos de entrada: no se usa para pantallas, solo si un futuro dispositivo necesita lectura |
| `enabled` | on | Deshabilitar el bus bloquea todos los dispositivos que cuelgan de él |

## Siguiente paso

Crea el bus y luego añade una [pantalla ST7735](/gekko/es/guides/displays/) y
abre el diseñador visual de disposición: páginas, widgets y marcadores vivos
funcionan igual que en el OLED.
