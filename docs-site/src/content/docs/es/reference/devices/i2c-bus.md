---
title: Bus I2C
description: "Cómo funciona el bus I2C: dos cables, direcciones de 7 bits, qué corre sobre él en Gekko y el diagnóstico integrado del bus."
sidebar:
  order: 4
  label: Bus I2C
---

## ¿Qué es I2C?

I2C (Inter-Integrated Circuit, «i-squared-C») es el bus de dos hilos de
cabecera de la electrónica de aficionado: una línea de **datos** (SDA) y una
línea de **reloj** (SCL), compartidas por todos los dispositivos. Igual que
en [1-Wire](/gekko/es/reference/devices/onewire-bus/), muchos dispositivos
coexisten sobre los mismos cables, pero aquí cada chip tiene una **dirección
de 7 bits** corta, normalmente impresa en su hoja de datos (y a menudo
seleccionable con puentes de soldadura).

En Gekko el bus es un dispositivo en sí mismo, `i2c_bus`: posee los dos pines,
realiza el escaneo de direcciones y cada periférico I2C que añades declara una
dependencia sobre él.

## Cableado

![Cableado I2C: SDA y SCL con pull-ups, OLED, HTU21, DS3231 y PCF8574 en paralelo, cada uno con su dirección](../../../../../assets/diagrams/i2c-wiring.svg)

Ambas líneas son open-drain y necesitan resistencias pull-up a 3,3 V. En la
práctica rara vez tienes que añadirlas tú: **prácticamente todas las placas
breakout (OLED, RTC, HTU21, expansores) ya las traen** y el dispositivo
`i2c_bus` habilita por defecto las pull-ups internas del ESP32. Solo un chip
desnudo en una línea larga necesita resistencias explícitas (2,2-10 kΩ).

Los dispositivos se conectan en paralelo: SDA con SDA, SCL con SCL, además de
3,3 V y GND. Mantén los cables razonablemente cortos (decenas de centímetros a
la velocidad por defecto): I2C es un bus de placa, no de cable largo como
1-Wire.

## Quién vive en el bus I2C en Gekko

| Dispositivo | Dirección típica |
| --- | --- |
| Pantalla OLED SSD1306 | `0x3C` (a veces `0x3D`) |
| AHT10 temperatura + humedad | `0x38` |
| HTU21 temperatura + humedad | `0x40` |
| Reloj en tiempo real DS3231 | `0x68` |
| Expansores PCF8574 / PCF8575 | `0x20`-`0x27` (seleccionable con puentes) |

Cada uno se crea como su propio dispositivo que depende del bus, con su
dirección en su propia config. Dos chips idénticos (por ejemplo, dos PCF8574
en distintas direcciones de puente) son simplemente dos dispositivos sobre el
mismo bus: Gekko rechaza crear dos dispositivos con la misma dirección en un
mismo bus.

## Escaneo y diagnóstico

La página del dispositivo tiene un botón **Scan bus**: sondea todas las
direcciones válidas y lista todo lo que responde, que es la forma más rápida
de confirmar el cableado y encontrar la dirección real de un módulo. Debajo
están los **diagnósticos del bus**: contadores consecutivos de error, el
último código de error y el estado de la transacción, con un botón de reset.
Un problema de cableado se ve aquí primero: los sensores sobre un bus enfermo
reportan `dependency_blocked` en vez de valores falsos.

![Ajustes del bus I2C con escaneo y diagnósticos](../../../../../assets/screenshots/device-i2c-bus.png)

## Configuración

| Campo | Valor por defecto | Significado |
| --- | --- | --- |
| `sdaPin` | `21` | Línea de datos (los pines I2C convencionales del ESP32 son 21/22) |
| `sclPin` | `22` | Línea de reloj |
| `frequencyHz` | `100000` | Velocidad del bus, 1-400 000 Hz; 100 kHz es el valor seguro por defecto, 400 kHz funciona con cableado corto |
| `internalPullup` | on | Usar las pull-ups internas del ESP32 (bien junto a las pull-ups de la placa) |
| `enabled` | on | Deshabilitar el bus bloquea todos los dispositivos que cuelgan de él |

## Solución de problemas

- **El escaneo no encuentra nada** - lo clásico es SDA/SCL cruzados; revisa
  también 3,3 V y GND en el módulo.
- **Dispositivo encontrado en otra dirección** - puentes (expansores) o una
  variante OLED `0x3D`; usa la dirección detectada.
- **Errores con carga/cables largos** - baja `frequencyHz` a 100 kHz, acorta
  el cableado y mantén los cables de pantalla lejos de relés y de la red.
