---
title: Schedule
description: "Referencia del tipo de dispositivo schedule de Gekko: reglas horarias por minuto y día de la semana."
sidebar:
  order: 8
---

`schedule` guarda un conjunto de reglas de tiempo y responde solo a una
pregunta: *¿está activo este horario ahora mismo?* No tiene salida propia:
úsalo como condición para un [auto switch](/gekko/es/guides/schedules-and-automation/)
(o para la programación de una bomba dosificadora) para que algo ocurra.

![Editor de reglas de schedule](../../../../../assets/screenshots/device-schedule.png)

## Dependencias

Ninguna. Otros dispositivos dependen del schedule, no al revés.

## Configuración

Hasta **4 reglas**, unidas por OR: el horario está activo cuando coincide
cualquier regla habilitada. Cada regla tiene:

| Campo | Significado |
| --- | --- |
| Días de la semana | Qué días de la semana se aplica la regla |
| Hora de inicio / fin | La ventana activa, en minutos del día (precisión de minuto: no hay segundos) |
| Modo | **Always on** - activo durante toda la ventana; **Interval** - divide la ventana en N tramos iguales, activo durante los primeros M minutos de cada uno |

El modo Interval cubre tareas periódicas: por ejemplo, una ventana de 08:00–
20:00 con 12 intervalos y 5 minutos de duración hace funcionar una bomba de
circulación 5 minutos cada hora.

## Tiempo y reloj

Las reglas se evalúan con el reloj propio del dispositivo y la zona horaria
configurada (el DST se maneja automáticamente). Hasta que el reloj sea
plausible - sincronizado por NTP o con un RTC DS3231 presente - el schedule se
reporta como no válido y los dispositivos dependientes mantienen sus salidas
seguras.

El editor del portal muestra una vista previa on/off y la siguiente
transición, calculada en tu navegador a partir de las mismas reglas; está
etiquetada como estimación porque el reloj y la zona horaria de tu navegador
pueden diferir de los del dispositivo.

## Proporciona

- **condition** - para auto switches y automatizaciones encadenadas.
- **schedule** - para dispositivos que consumen horarios directamente.
