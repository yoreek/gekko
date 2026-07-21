---
title: Termostato
description: "Cómo el termostato de Gekko mantiene la temperatura en un punto de consigna: el lazo de control, la histéresis y las barreras de seguridad."
sidebar:
  order: 7
---

## Qué hace

Un termostato cierra el lazo entre un sensor de temperatura y un interruptor:
*si el agua está demasiado fría, enciende el calentador; cuando está lo
suficientemente caliente, lo apaga*. En Gekko eso es un dispositivo
`thermostat` conectado a otros dos:

![Lazo de control: DS18B20 mide, el termostato decide, el relé alimenta el calentador, el agua se calienta, repetir](../../../../../assets/diagrams/thermostat-loop.svg)

También funciona para enfriar: el modo **cool** gobierna un enfriador o un
ventilador con la misma lógica invertida, y **off** deja la salida en reposo.

## Histéresis: por qué no oscila

Un ingenuo "on por debajo de 25,0, off por encima de 25,0" haría vibrar el
relé decenas de veces por minuto mientras la lectura se mueve alrededor del
punto de consigna. La solución es una **banda muerta**: la histéresis.

![Gráfica de histéresis: calentador on por debajo de 24,5, off a 25,0, nada cambia dentro de la banda](../../../../../assets/diagrams/thermostat-hysteresis.svg)

Con objetivo 25,0 °C e histéresis 0,5 °C en modo heat:

- el calentador se enciende cuando la temperatura baja a **24,5** (objetivo
  menos histéresis);
- permanece encendido hasta que la temperatura llega a **25,0**, entonces se
  apaga;
- entre medias no cambia nada: la temperatura puede derivar por la banda.

Más histéresis = menos ciclos de relé pero mayor oscilación térmica; menos
= control más ajustado pero más conmutación. Para un calentador de acuario,
0,3-0,5 °C es un rango sensato. Además, **min switch interval** (por defecto
5 s) impone un límite duro entre cambios de salida: seguro barato para
relés, esencial para enfriadores con compresor, que no deben ciclarse
demasiado rápido.

## Barreras de seguridad

El termostato asume que a veces algo saldrá mal y falla hacia "heater off":

- **Rango seguro** (`minSafeCelsius` / `maxSafeCelsius`) - una lectura fuera de
  esta ventana se trata como fallo (el sensor se salió del agua, el cable se
  rompió a un valor fijo): la salida pasa a su estado seguro y el estado
  muestra `out_of_range`.
- **Timeout del sensor** - no recibir una lectura fresca dentro de
  `sensorTimeoutMs` (bus muerto, sensor deshabilitado) también detiene la
  calefacción: `sensor_timeout`.
- **Reintento con back-off** - tras un error el termostato espera
  `retryAfterErrorMs` antes de intentar de nuevo, en vez de golpear cada
  segundo un sensor roto.
- El **estado seguro propio del interruptor** cubre el fallo inverso: si el
  termostato se deshabilita o se borra, el
  [interruptor vuelve](/gekko/es/reference/devices/gpio-switch/) al estado que
  configuraste allí.

## Configurarlo

1. Crea el [sensor DS18B20](/gekko/es/reference/devices/ds18b20/) (o NTC/HTU21).
2. Crea el [interruptor](/gekko/es/reference/devices/gpio-switch/) que gobierna
   el relé del calentador. Los calentadores son un caso en el que conviene
   pensar en `safeState: off` y `startupState: off`.
3. Crea el **termostato**: elige sensor y interruptor, fija modo, objetivo e
   histéresis.

![Ajustes del termostato en el portal](../../../../../assets/screenshots/device-thermostat.png)

## Configuración

| Campo | Valor por defecto | Significado |
| --- | --- | --- |
| `mode` | `heat` | `heat`, `cool` u `off` |
| `targetCelsius` | `25` | El punto de consigna |
| `hysteresisCelsius` | `0.5` | La banda muerta por debajo (heat) o por encima (cool) del objetivo |
| `minSafeCelsius` / `maxSafeCelsius` | `0` / `50` | Límites de fallo para la lectura del sensor |
| `checkIntervalMs` | `1000` | Periodo del lazo de control |
| `sensorTimeoutMs` | `6000` | Edad máxima de una lectura antes de `sensor_timeout` |
| `minSwitchIntervalMs` | `5000` | Tiempo mínimo entre cambios de salida |
| `retryAfterErrorMs` | `30000` | Espera antes de reintentar tras un error |

## Runtime y Home Assistant

El runtime reporta la temperatura actual, el estado de salida y un estado -
`heating`, `cooling`, `idle`, `sensor_timeout`, `out_of_range`,
`dependency_blocked` - que se muestra con iconos en el portal y se registra en
el diario de eventos del dispositivo. En
[compilaciones MQTT](/gekko/es/guides/mqtt-home-assistant/) el termostato aparece
en Home Assistant como una entidad `climate` completa (modo, consigna,
temperatura actual, acción), y los cambios de consigna desde HA se validan
contra el rango seguro.
