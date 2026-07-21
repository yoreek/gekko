---
title: Salidas analógicas y compositor de luces
description: Salidas PWM regulables en Gekko: transiciones suaves, curvas diarias de brillo y luminarias multicanal como una luz de acuario de cinco canales.
sidebar:
  order: 10
---

## ¿Por qué salidas regulables?

Un interruptor on/off sirve para un calentador, pero una luz no debería pasar
de 0 a 100 % a las 9 de la mañana de golpe. Una buena luz de acuario (o de
terrario o invernadero):

- **sube y baja gradualmente** - amanecer y atardecer, no un interruptor; los
  peces se sobresaltan visiblemente con cambios bruscos, y a los corales
  tampoco les gustan;
- **cambia de intensidad a lo largo del día** - un pico al mediodía, mañanas y
  tardes más suaves;
- **mezcla varios canales de color** - las luminarias reef suelen tener tiras
  separadas de royal blue, blue, white, violet y moonlight, cada una con su
  propia curva diaria.

Gekko modela esto con cuatro tipos de dispositivo que encajan como bloques.
Cada nivel es un porcentaje (0-100 %) en el portal y la API; la parte de
hardware es un pin PWM (LEDC) del ESP32 que gobierna la entrada de dimmer de un
driver LED, un módulo MOSFET o cualquier otra carga controlada por PWM.

## Los bloques

| Tipo | Qué hace |
| --- | --- |
| `analog_output` | El canal PWM de hardware en un pin |
| `fade_analog_output` | Suaviza cada cambio hasta convertirlo en una rampa gradual |
| `scheduled_analog_output` | Lleva su objetivo a lo largo de una curva diaria |
| `analog_output_composer` | Agrupa varios canales en una sola luminaria |

Una salida con fade o programada toma exactamente una dependencia de rol
`analog_output` y *proporciona ese mismo rol ella misma*, así que se pueden
apilar:

![Cadena decoradora: la salida programada calcula el nivel, fade suaviza, analog output escribe PWM](../../../../../assets/diagrams/analog-chain.svg)

- **Fade** - `maxStep` (porcentaje por paso) y `stepIntervalMs` fijan la
  velocidad de la rampa; el valor por defecto de ~1 % cada 200 ms convierte
  cualquier cambio, incluso mover un slider manual, en una transición suave.
- **Scheduled** - hasta 10 puntos `(time, level)` al día, interpolados entre
  puntos. Modos: **Off**, **Manual** (nivel fijo), **Scheduled** (seguir la
  curva). Sin un reloj válido, la salida cae a cero en vez de mantener un
  nivel antiguo.

El registro impone que cada salida tenga **como mucho un controlador**: no
puedes cablear accidentalmente dos horarios al mismo canal.

## Ejemplo completo: una luz de acuario de cinco canales

El objetivo: un día que se vea así:

![Curvas diarias de cinco canales: los azules suben primero y se quedan más tiempo, el blanco alcanza su pico al mediodía, el violeta añade acentos y el moonlight brilla de noche](../../../../../assets/diagrams/aquarium-light-day.svg)

Los azules suben primero y caen los últimos (los corales fotosintetizan sobre
todo en azul), el blanco cálido llena el mediodía, el violeta añade el toque
fluorescente y un canal moonlight tenue brilla por la noche. Para construirlo:

1. Crea cinco dispositivos **`analog_output`**, uno por pin de driver LED:
   "Royal blue LEDC", "Blue LEDC", "White LEDC", "Violet LEDC", "Moonlight
   LEDC".
2. Envuélvelos a todos en un **`fade_analog_output`** ("Royal blue fade" →
   objetivo "Royal blue LEDC", ...) para que los cambios de canal nunca den
   saltos.
3. Envuelve cada fade en un **`scheduled_analog_output`** ("Royal blue
   schedule" → objetivo "Royal blue fade", ...) y dibuja la curva diaria de
   ese canal.
4. Crea un **`analog_output_composer`** "Aquarium light" y añade los cinco
   outputs programados como sus canales.

![Compositor de luz de acuario en el portal](../../../../../assets/screenshots/device-analog-composer.png)

El compositor ahora se comporta como *la* luz:

- **Un modo para toda la luminaria** - cambiar el compositor entre Off / Manual
  / Scheduled empuja ese modo a todos los canales y los mantiene sincronizados
  si alguno se desvía. Off pone todo a cero.
- **Un editor** - todas las curvas de los canales en una sola gráfica, editadas
  en sitio arrastrando puntos (clic derecho para insertar/eliminar, snapping
  opcional de 15 min/5 %, un fondo de amanecer/atardecer como referencia); el
  modo manual muestra un slider por canal.
- **Una sola tarjeta de panel** - fija el compositor para una vista previa
  compacta de la programación multicanal.

El compositor solo hace falta cuando varios canales deben actuar como una
sola luminaria: una luz de un solo canal es simplemente los pasos 1-3 con una
sola cadena. Omite la capa de fade si no te importan las rampas.

## Runtime y control

Los cuatro tipos reportan su nivel en vivo (los fades también reportan el
objetivo y si siguen transitando). Un slider del panel o el comando `setOutput`
gobiernan un canal directamente; los cambios de modo van por `setMode`. En
[compilaciones MQTT](/gekko/es/guides/mqtt-home-assistant/) los canales son
detectables en Home Assistant. Internales:
[`docs/analog-output.md`](https://github.com/yoreek/gekko/blob/master/docs/analog-output.md).
