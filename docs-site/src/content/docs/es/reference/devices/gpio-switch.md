---
title: Interruptor GPIO
description: "Referencia del tipo de dispositivo gpio_switch de Gekko: una salida on/off en un pin GPIO del ESP32."
sidebar:
  order: 2
---

`gpio_switch` controla un pin GPIO como salida on/off: placas de relés,
módulos MOSFET, LEDs de estado. Suele ser el primer dispositivo que creas; el
[recorrido del primer dispositivo](/gekko/es/getting-started/first-device/) usa
este tipo.

![Ajustes del interruptor GPIO](../../../../../assets/screenshots/device-gpio-switch.png)

## Dependencias

Ninguna: posee directamente su pin GPIO. (Para salidas detrás de un expansor
PCF8574/PCF8575, usa `port_expander_switch`; ofrece las mismas opciones de
interruptor que verás abajo.)

## Configuración

| Campo | Valor por defecto | Significado |
| --- | --- | --- |
| `gpioPin` | `4` | El pin de salida |
| `inverted` | off | Invierte el nivel eléctrico - actívalo para placas de relé activas en bajo |
| `startupState` | off | Estado de salida justo después del arranque (si no se restaura el anterior) |
| `restorePreviousState` | off | Restaura el último estado anterior al reinicio en vez de `startupState` |
| `safeState` | off | Estado al que caer cuando un dispositivo controlador deja de estar disponible |
| `enabled` | on | Los dispositivos deshabilitados liberan su salida y dejan de reportar |

## Runtime y control

El dispositivo reporta su estado vivo on/off; puedes conmutarlo desde la lista
de dispositivos, un widget de interruptor en el panel, la API REST o Home
Assistant (como entidad `switch` en
[compilaciones MQTT](/gekko/es/guides/mqtt-home-assistant/)).

## Proporciona

- **switch** - puede ser gobernado por un termostato, un auto switch o una
  bomba dosificadora.
- **condition** - su estado on/off puede bloquear un auto switch.
