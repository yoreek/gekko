---
title: Tira de píxeles (WS2812B)
description: Tiras RGB direccionables WS2812B en Gekko — un backend de hardware más efectos de color sólido y parpadeo de alerta, ambos controlables en vivo y desde Home Assistant.
sidebar:
  order: 13
---

## Los bloques de construcción

Una tira direccionable es más que una salida on/off o atenuable — es un
array de colores. Gekko lo modela con un backend de hardware y dispositivos
de efecto que lo controlan, el mismo patrón decorador que las
[salidas analógicas](/gekko/es/reference/devices/analog-outputs/):

| Tipo | Qué hace |
| --- | --- |
| `pixel_strip` | Un pin de datos WS2812B — posee el búfer de píxeles y lo escribe al hardware |
| `pixel_effect_solid` | Rellena la tira objetivo con un color estático |
| `pixel_effect_alert` | Hace parpadear la tira objetivo mientras se cumplen sus condiciones |

Cada efecto toma exactamente una dependencia `pixel_strip` y la mantiene
**en exclusiva** — no se pueden conectar dos efectos a la misma tira a la
vez, así que nunca compiten por ella. Los efectos aún no se encadenan entre
sí (a diferencia de fade/scheduled en salidas analógicas); cada tira ejecuta
un único efecto a la vez.

## `pixel_strip`

El dispositivo de hardware. Configuración:

- **Pin** — el GPIO cableado a la línea de datos de la tira.
- **Número de píxeles** — cuántos LED tiene la tira (hasta 300).
- **Brillo de arranque** — el brillo aplicado al arrancar cuando no hay un
  estado retenido que restaurar.
- **Restaurar estado anterior** — arrancar con el último brillo en vivo en
  lugar de arrancar siempre con el valor de arranque configurado.

El brillo y el on/off son **estado en vivo**, no forman parte de la
configuración guardada — mover el control deslizante del panel o apagar el
dispositivo nunca marca la configuración como modificada ni pide un diálogo
de guardado. Apagarlo siempre muestra negro en el hardware; volver a
encenderlo restaura el último brillo que estaba puesto, así que nunca tienes
que volver a introducir un valor.

## `pixel_effect_solid`

Rellena su tira objetivo con un color y lo mantiene — la forma más simple de
iluminar una tira con un único tono (un canal de luz de luna, una luz de
acento, un blanco estático de arrecife).

- El selector de **color** fija el color en vivo directamente desde el
  widget; el selector de color del formulario de configuración solo fija el
  **color de arranque** aplicado al arrancar.
- **Restaurar estado anterior** funciona exactamente igual que en
  `pixel_strip`: restaurar el último color en vivo, o arrancar siempre desde
  el color de arranque.
- La misma compuerta explícita de on/off que `pixel_strip` — apagado
  siempre muestra negro en el hardware sin importar el color configurado,
  independientemente del color que esté guardado.

## `pixel_effect_alert`

Hace parpadear su tira objetivo entre un **color** configurado y negro a un
**intervalo de parpadeo** configurado, mientras una lista acotada de hasta 4
dispositivos con rol `Condition` (un horario, un interruptor, un auto
switch, …) se cumplen todos a la vez — el mismo mecanismo de condiciones (Y)
que usa [`auto_switch`](/gekko/es/guides/schedules-and-automation/). Una
lista de condiciones vacía nunca se cumple, así que una alerta mal
configurada no puede parpadear por accidente. A diferencia de
`pixel_strip`/`pixel_effect_solid`, aquí el color y el intervalo de parpadeo
son configuración persistida normal — el color y el ritmo de una alerta
describen lo que la alerta *significa*, no un valor que se ajustaría en vivo.

Uso típico: conectar un interruptor de flotador de desbordamiento o la
condición derivada de un `binary_sensor` de fuga a una tira de alerta roja
junto al acuario.

## Tiempo de ejecución y control

`pixel_strip` reporta su brillo en vivo y su número de píxeles;
`pixel_effect_alert` reporta si sus condiciones se cumplen actualmente. Un
control deslizante o selector de color del panel controla brillo/color
directamente.

En [builds con MQTT](/gekko/es/guides/mqtt-home-assistant/), los tres tipos
son detectables en Home Assistant: `pixel_strip` y `pixel_effect_solid` se
publican cada uno como un `light` (solo brillo y solo RGB,
respectivamente), y `pixel_effect_alert` se publica como un
`binary_sensor`. Internos:
[`docs/pixel-strip.md`](https://github.com/yoreek/gekko/blob/master/docs/pixel-strip.md).
