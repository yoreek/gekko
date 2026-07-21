---
title: Horarios y automatización
description: Horarios por tiempo del día y auto switches guiados por condiciones en Gekko.
sidebar:
  order: 2
---

Dos tipos de dispositivo trabajan juntos para automatizar conmutaciones: un
**schedule** mantiene reglas basadas en horas y reporta si está activo en ese
momento, y un **auto switch** gobierna un interruptor real a partir del AND
lógico de sus condiciones adjuntas.

## Schedule

Un [dispositivo schedule](/gekko/es/reference/devices/schedule/) mantiene hasta 4
reglas. Cada regla tiene:

- una **máscara de días** - qué días se aplican;
- una **ventana horaria** - minuto de inicio y fin del día (precisión de
  minuto, sin segundos);
- un **modo**:
  - **Always on** - activo durante toda la ventana;
  - **Interval** - divide la ventana en tramos iguales y está activo durante
    los primeros N minutos de cada tramo (para circulación, nebulización, etc.).

El horario está activo cuando **cualquier** regla habilitada coincide. El
editor de reglas del portal muestra una vista previa de encendido/apagado
calculada en el navegador a partir de las reglas y del reloj del navegador,
marcada como estimación porque el dispositivo evalúa las reglas con su propio
reloj y su propia zona horaria.

:::note[Dale al dispositivo un reloj fiable]
Los horarios no hacen nada hasta que el reloj del dispositivo es plausible.
Usa NTP (ajusta la zona horaria en la página **Time**) o añade un dispositivo
RTC DS3231 para que los horarios sobrevivan a cortes de Internet y reinicios.
:::

## Auto switch

Un auto switch envuelve un interruptor real (GPIO o port-expander) y lo
gobierna a partir de hasta **6 dependencias de condición** - horarios, otros
interruptores o incluso otros auto switches - cada una opcionalmente
**invertida**. Todas las condiciones se combinan con AND: la salida solo está
encendida cuando todas se cumplen. Sin condiciones adjuntas, un auto switch en
modo Auto permanece apagado.

Sus modos son:

- **Off / On** - anulación manual; las condiciones se ignoran. Cambiarlo
  desde el panel establece exactamente esto.
- **Auto** - seguir las condiciones.
- **Paused** - apagado temporal durante una duración configurada, y luego
  vuelve automáticamente a **Auto**. La pausa solo está disponible desde modo
  Auto. Un reinicio en mitad de la pausa la reanuda con el tiempo restante
  correcto.

Al entrar en Auto (o Paused), el interruptor objetivo se fuerza siempre a
apagado primero y después se entrega a las condiciones, para que un "On"
manual anterior no se quede pegado silenciosamente.

Como un auto switch también actúa como switch y como condition, puedes
encadenar automatizaciones: un auto switch de "feeding mode" puede bloquear
varios auto switches a la vez a través de sus ranuras de condición invertidas.

## Ejemplo: luz de acuario con botón de pausa

1. Crea un **Schedule** "Light hours", regla: todos los días, 09:00–21:00,
   always on.
2. Crea un **GPIO Switch** "Light relay" en el pin que controla la luz.
3. Crea un **Auto Switch** "Light": target = "Light relay", condition =
   "Light hours", mode = Auto.
4. Fija "Light" en el panel. Ahora sigue el horario; tócalo para una anulación
   manual, usa **pause** durante el mantenimiento y vuelve a Auto cuando
   termines.

## Dispositivos relacionados

- **[Thermostat](/gekko/es/reference/devices/thermostat/)** - control de
  temperatura con histéresis que gobierna un interruptor.
- **[Dosing pump](/gekko/es/reference/devices/dosing-pump/)** - dosificación
  programada con calibración, contabilidad del depósito y un diario de dosis.
- **[Scheduled analog output](/gekko/es/reference/devices/analog-outputs/)** -
  una curva diaria de brillo/nivel para salidas PWM, combinable en
  luminarias multicanal.
