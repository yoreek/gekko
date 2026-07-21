---
title: Dispositivos y dependencias
description: Cómo funcionan el registro tipado de dispositivos de Gekko y su grafo de dependencias.
sidebar:
  order: 1
---

La idea central de Gekko es un **registro de dispositivos**: una lista
persistida de instancias de dispositivo, cada una creada a partir de uno de
los [tipos de dispositivo](/gekko/es/reference/devices/) incorporados, con su
propia configuración y estado en tiempo real.

## Los dispositivos se componen, no se configuran de forma aislada

El hardware real está en capas: un sensor va sobre un bus, un interruptor va
detrás de un expansor de puertos, una automatización gobierna un interruptor.
Gekko lo modela directamente: un dispositivo **declara dependencias** sobre
otros dispositivos, por rol. Ejemplos:

| Este dispositivo... | ...depende de |
| --- | --- |
| Sonda de temperatura DS18B20 | un dispositivo de bus 1-Wire (que posee el GPIO) |
| Pantalla OLED SSD1306 | un dispositivo de bus I2C |
| Interruptor en un PCF8574 | el dispositivo expansor de puertos |
| Termostato | un sensor de temperatura **y** un interruptor |
| Auto switch | un interruptor real, más hasta 6 dispositivos de condición |
| Salida analógica programada | un canal de salida analógica |

El registro valida el grafo cuando creas o editas un dispositivo: no puedes
conectar una pantalla a un dispositivo que no sea un bus I2C, y no puedes
borrar un bus mientras un sensor siga dependiendo de él. Las dependencias se
eligen en los diálogos del portal a partir de listas ya filtradas a dispositivos
compatibles.

## Roles, no pares fijas

Las dependencias se emparejan por **rol** (`switch`, `temperature_sensor`,
`i2c_bus`, `condition`, ...), y un tipo de dispositivo puede proporcionar
varios roles. Un interruptor GPIO es a la vez `switch` y `condition`, así que
un auto switch puede usarlo tanto como salida que gobierna como condición de
entrada. Un auto switch también proporciona `switch` y `condition`, así que
las automatizaciones se pueden encadenar.

## Configuración vs. estado en ejecución

Cada dispositivo separa:

- **Config** - ajustes persistidos (nombre, pines, reglas, dependencias). Se
  guarda en el dispositivo en formato binario versionado y se migra
  automáticamente entre actualizaciones de firmware. Esto es lo que contienen
  los [paquetes de copia de seguridad](/gekko/es/guides/backup-restore/).
- **Runtime** - estado vivo (encendido/apagado, temperatura, estado como
  `ready` o `dependency_blocked`). Nunca se guarda dentro de la config; se
  emite al portal por WebSocket en tiempo real.

Algunos tipos además conservan un pequeño **estado retenido** entre reinicios -
por ejemplo el último estado de salida de un interruptor (cuando "restore
previous state" está activado) o la cuenta atrás pausada de un auto switch -
sin reescribir la configuración.

## Ciclo de vida

Los dispositivos pueden **habilitarse/deshabilitarse** sin borrarlos, y cada
instancia expone un estado que el portal muestra: un sensor con un bus
ausente muestra `dependency_blocked`, un dispositivo en fallo muestra su
error, y el diario de **Device events** registra las transiciones.
