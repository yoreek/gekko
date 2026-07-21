---
title: Bomba dosificadora
description: Qué es una bomba dosificadora, por qué los acuaristas automatizan la dosificación y cómo dosing_pump de Gekko planifica, calibra y registra cada mililitro.
sidebar:
  order: 9
---

## ¿Qué es una bomba dosificadora?

Una bomba dosificadora es una bomba de líquido lenta y precisa. El tipo más
habitual es **peristáltico**: un motor pequeño aprieta un tubo de silicona
blando con rodillos y empuja unos pocos mililitros por segundo. Como el
líquido solo toca el tubo, la bomba no puede contaminarlo; y como el caudal es
estable, *el tiempo de funcionamiento se traduce directamente en mililitros*.

Los acuaristas (y los jardineros) la usan allí donde un líquido debe añadirse
**poco y a menudo**:

- **tanques reef** - calcio, alcalinidad (KH), magnesio, oligoelementos. Los
  corales consumen esto continuamente; las microdosis diarias mantienen la
  química mucho más estable que una corrección semanal grande.
- **tanques plantados** - fertilizante líquido diario en vez de "cuando me
  acuerdo".
- **estanques/invernaderos** - buffer de pH, nutrientes.

La dosificación manual implica probetas, calendario y días olvidados. Una
bomba automatizada hace el mismo trabajo cada día a la misma hora: esa
consistencia es precisamente el objetivo.

## Las piezas y cómo las une Gekko

![Configuración de bomba dosificadora: depósito con sensor de flotador, bomba peristáltica gobernada por un relé, ESP32, acuario](../../../../../assets/diagrams/dosing-setup.svg)

Necesitas cuatro piezas baratas, y cada una corresponde a un dispositivo de
Gekko:

| Hardware | Dispositivo Gekko | Rol |
| --- | --- | --- |
| Bomba peristáltica + placa relé/MOSFET (cable naranja arriba) | `gpio_switch` (o `port_expander_switch`) | La salida que el dispositivo de bomba activa y desactiva |
| La bomba dosificadora en sí (lógica) | `dosing_pump` | Posee el horario, la calibración, el depósito y el historial |
| Botella/depósito con la solución | - (seguido por la config de `dosing_pump`) | De dónde estás dosificando |
| Sensor de flotador opcional en la botella (cable verde arriba) | `binary_sensor` | Le dice a Gekko que la botella está vacía, independientemente del contador |

¿Varias bombas? Crea una cadena por líquido: un soporte reef típico lleva dos
o tres (por ejemplo calcio, alcalinidad, magnesio) lado a lado, cada una con
su botella y su horario.

## Configurarlo

1. Crea un **GPIO switch** en el pin que gobierna el relé de la bomba (ver
   [tu primer dispositivo](/gekko/es/getting-started/first-device/): es el mismo
   flujo).
2. Opcionalmente crea un **binary sensor** para el flotador.
3. Crea el dispositivo **dosing pump**: elige el interruptor como *pump
   switch*, el sensor como *low-level sensor* (invertible por enlace), fija la
   capacidad del depósito y el umbral de aviso, y añade ranuras de dosis al
   horario.

![Ajustes de la bomba dosificadora en el portal](../../../../../assets/screenshots/device-dosing-pump.png)

## Calibra antes de confiar en ella

Gekko convierte mililitros en segundos de funcionamiento mediante un solo
número: el caudal de la bomba (`ml/s`). La longitud del tubo, el diámetro y la
altura de impulsión lo cambian, así que mídelo una vez con tu instalación real:

![Calibración: ejecutar una dosis, medir el volumen real, introducirlo](../../../../../assets/diagrams/dosing-calibration.svg)

Las ejecuciones de calibración se excluyen de estadísticas e historial, pero
el líquido dispensado **sí** se descuenta del depósito: realmente salió de la
botella. Si ya conoces el caudal, un modo directo te deja introducirlo a mano.

## Programación: cómo se disparan realmente las dosis

El horario mantiene ranuras de dosis (hora del día + cantidad) y un patrón de
días: cada N días o días concretos de la semana. El dispositivo lo evalúa con
su propio reloj, así que
[dale una fuente de tiempo fiable](/gekko/es/reference/devices/schedule/#tiempo-y-reloj).
El total diario se reparte a propósito en varias dosis pequeñas: estabilidad
otra vez.

![Línea temporal de dosis: dosis puntual, dosis dentro de la ventana de gracia de 5 minutos, dosis perdida que se omite](../../../../../assets/diagrams/dosing-timeline.svg)

Dos políticas merecen entenderse:

- **Ventana de gracia.** Una ranura puede empezar hasta 5 minutos tarde: por
  ejemplo, si una dosis manual o una calibración ocupaba la bomba en el minuto
  programado.
- **Omitir, no dosificar tarde.** Una ranura perdida más allá de la ventana de
  gracia se *omite*, nunca se aplaza. Después de un reinicio, de que el reloj
  se sincronice a mitad del día o de una calibración larga, **no** obtendrás
  un aluvión de dosis atrasadas: para la química del agua, un aluvión tardío es
  peor que una microdosis perdida.

Además, por ranura: **skip next** suprime exactamente una ocurrencia futura
(cambio de agua, vacaciones), y el interruptor **auto** bloquea todo el
programador mientras las dosis manuales siguen funcionando.

Una dosis, una vez iniciada, corre enteramente en el dispositivo: el portal
solo envía el comando. Puedes cerrar el navegador en mitad de la dosis; el
firmware mide el tiempo, apaga la bomba y registra la cantidad dispensada. Solo
puede haber una ejecución activa a la vez (manual, programada o calibración:
ninguna preempta a otra), y cualquier cosa que saque el dispositivo de servicio
fuerza la parada del motor.

## Seguimiento del depósito

Dile a Gekko la capacidad de la botella y contará cada mililitro:

![Seguimiento del depósito: el contador baja, la alerta de bajo nivel, vacío bloquea la dosificación automática](../../../../../assets/diagrams/dosing-container.svg)

- Por debajo del **umbral de aviso** el portal lanza una alerta (campana +
  toast): hora de mezclar una nueva tanda.
- **Vacío** - contador a cero o flotador activado - lanza una alerta crítica,
  y con **block auto dosing when empty** activado, las dosis programadas se
  detienen en vez de hacer trabajar la bomba en seco.
- **`daysLeft`** proyecta cuánto dura el volumen restante al consumo diario
  medio del horario.
- Tras rellenar, regístralo con el comando **Set volume**.

## Diario de dosis

Cada dosis programada y manual se añade a un diario en el dispositivo: más de
90 días de historial con ritmos típicos de dosificación, guardado en una
partición flash dedicada para que sobreviva a actualizaciones de firmware y
del portal. La página del dispositivo lo grafica; `GET
/api/dosejournal?deviceId=<id>&periodDays=<n>` lo sirve en bruto. El diario es
un anillo de tamaño fijo por bomba: los registros viejos se van rotando solos
y el historial de una bomba nunca puede comerse el de otra.

## Comandos (REST)

| Comando | Payload | Efecto |
| --- | --- | --- |
| `startDose` | `amountMl`, `logging` | Dosis manual (`logging:false` = ejecución de calibración) |
| `stopDose` | - | Parar ya, registrar la cantidad real |
| `setVolume` | `volumeMl` | Relleno/corrección del depósito |
| `skipNext` | `doseIndex`, `skip` | Omitir una futura ocurrencia de una ranura |
| `setMode` | `auto` / `manual` | Activar/desactivar el programador |

Modelo completo de ejecución e internos del diario:
[`docs/dosing-pump.md`](https://github.com/yoreek/gekko/blob/master/docs/dosing-pump.md).
