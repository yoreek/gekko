---
title: Expansores de puertos (PCF8574 / PCF8575)
description: "Añadir más salidas de conmutación por I2C en Gekko: los expansores PCF8574 y PCF8575 y el port_expander_switch que controla uno de sus pines."
sidebar:
  order: 2.5
---

## ¿Por qué un expansor de puertos?

El ESP32 tiene bastantes GPIO, pero un acuario ocupado aún puede quedarse sin
pines: cada relé, cada MOSFET quiere uno, y algunos ya están ocupados (el bus
I2C, una pantalla SPI, los pines ADC de solo entrada). Un **expansor de
puertos** es la solución barata: un chip I2C que te da **8 o 16 pines de I/O
extra usando los mismos dos cables** que ya comparten el resto de dispositivos
I2C. Cableas un expansor y has añadido un banco de salidas para relés sin
tocarle otro pin al ESP32.

Gekko soporta los dos más ubicuos:

| Tipo | Chip | Pines extra | Direcciones |
| --- | --- | --- | --- |
| `pcf8574_expander` | PCF8574 | 8 | `0x20`-`0x27` |
| `pcf8575_expander` | PCF8575 | 16 | `0x20`-`0x27` |

Ambos son hubs de rol `port_expander` sobre un
[bus I2C](/gekko/es/reference/devices/i2c-bus/); la única diferencia es 8 frente
a 16 pines. Hasta ocho de cada uno pueden compartir un bus, con sus
direcciones fijadas por los puentes A0/A1/A2.

## Hub y canales

Igual que en los [hubs ADS1115/multiplexor](/gekko/es/reference/devices/analog-inputs/),
un expansor es un **hub**: el dispositivo expansor posee el chip, y cada pin de
salida que realmente usas es un dispositivo separado
**`port_expander_switch`** que depende de él.

Así que un montaje de dos relés con PCF8574 tiene tres dispositivos: el
`pcf8574_expander` y dos `port_expander_switch` (pin 0 y pin 1) que apuntan a
él. Cada interruptor se nombra, habilita y controla de forma independiente, y
se comporta exactamente como un
[GPIO switch](/gekko/es/reference/devices/gpio-switch/), solo que sobre un pin del
expansor en vez de un pin del ESP32. Dos interruptores no pueden reclamar el
mismo pin en un mismo expansor; Gekko rechaza el segundo.

Como un `port_expander_switch` **proporciona el rol `switch`**, cualquier cosa
que gobierne un interruptor también lo gobierna a él: un
[termostato](/gekko/es/reference/devices/thermostat/), un `auto_switch`, una
[bomba dosificadora](/gekko/es/reference/devices/dosing-pump/). Nada en esos
controladores sabe ni necesita saber que la salida está detrás de un
expansor.

## Configurarlo

1. Crea un **[bus I2C](/gekko/es/reference/devices/i2c-bus/)** (si no tienes uno)
   y usa **Scan bus** para confirmar que el expansor responde, normalmente en
   `0x20`.
2. Crea un **`pcf8574_expander`** (o `pcf8575_expander`), selecciona ese bus
   y fija su dirección.
3. Para cada salida, crea un **`port_expander_switch``, selecciona el
   expansor y elige el número de pin (0-7 en un PCF8574, 0-15 en un PCF8575).

![Ajustes del expansor PCF8574: bus I2C, dirección con escaneo y la opción de polaridad](../../../../../assets/screenshots/device-pcf8574-expander.png)

Luego el interruptor en sí, con las mismas opciones que cualquier GPIO switch:

![Ajustes del interruptor del expansor de puertos: selector de expansor, número de pin y opciones del interruptor](../../../../../assets/screenshots/device-port-expander-switch.png)

## Placas de relé activas en bajo

La mayoría de las placas de relé baratas son **activas en bajo**: el relé se
cierra cuando el pin se tira a *bajo*, no a alto. Hay dos sitios donde
corregirlo, y conviene hacerlo con intención:

- **En el expansor** - su opción `inverted` invierte la polaridad eléctrica de
  *todo el chip*. Úsala cuando toda la placa sea activa en bajo.
- **En el interruptor** - su opción `inverted` invierte un *solo pin*. Úsala
  cuando solo algunos pines estén cableados en activo bajo.

Hazlo bien y "on" en Gekko significará que el relé está realmente energizado.

## Configuración

### `pcf8574_expander` / `pcf8575_expander`

| Campo | Valor por defecto | Significado |
| --- | --- | --- |
| `i2cAddress` | `0x20` | Dirección del chip (`0x20`-`0x27` por los puentes A0/A1/A2) |
| `inverted` | off | Invierte el nivel eléctrico de todos los pines (placas activas en bajo) |
| `enabled` | on | Deshabilitar el expansor libera todos los interruptores que cuelgan de él |

### `port_expander_switch`

| Campo | Valor por defecto | Significado |
| --- | --- | --- |
| `channel` | `0` | Qué pin del expansor (0-7 en PCF8574, 0-15 en PCF8575) |
| `inverted` | off | Invierte el nivel eléctrico de este pin concreto |
| `startupState` | off | Estado de salida justo después del arranque (si no se restaura el anterior) |
| `restorePreviousState` | off | Restaura el último estado anterior al reinicio en vez de `startupState` |
| `safeState` | off | Estado al que caer cuando un dispositivo controlador deja de estar disponible |
| `enabled` | on | Los interruptores deshabilitados liberan su pin y dejan de reportar |

## Proporciona

Un `port_expander_switch` proporciona los mismos roles que un GPIO switch:

- **switch** - puede ser gobernado por un termostato, un auto switch o una
  bomba dosificadora;
- **condition** - su estado on/off puede bloquear un auto switch.

En [compilaciones MQTT](/gekko/es/guides/mqtt-home-assistant/) cada interruptor
es detectable en Home Assistant como entidad `switch`. El expansor en sí no
lo es: proporciona pines, no un control propio.
