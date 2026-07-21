---
title: Bus 1-Wire
description: Cómo funciona el bus 1-Wire: cableado, resistencia pull-up, direcciones ROM de 64 bits y cuántos sensores comparten un solo pin del ESP32.
sidebar:
  order: 3
  label: Bus 1-Wire
---

## ¿Qué es 1-Wire?

1-Wire es un bus serie (originalmente de Dallas Semiconductor) construido
alrededor de una idea radical: **una sola línea de datos compartida para
todo**. El controlador y cualquier número de dispositivos - en la práctica,
sondas de temperatura DS18B20 - cuelgan del mismo cable, y cada dispositivo
se direcciona individualmente. Por eso todo un acuario de sondas de
temperatura te cuesta exactamente **un pin GPIO**.

En Gekko, el propio bus es un dispositivo: `onewire_bus`. Posee el pin, hace
el escaneo y cada [sensor DS18B20](/gekko/es/reference/devices/ds18b20/) que
crees depende de él.

## Cableado: tres cables y una resistencia

![Cableado 1-Wire: 3V3, GND, DATA con una pull-up de 4,7 kΩ entre DATA y 3V3](../../../../../assets/diagrams/onewire-wiring.svg)

La sonda impermeable DS18B20 típica tiene tres conductores:

| Conductor | Color habitual | Conectar a |
| --- | --- | --- |
| VDD | rojo | 3,3 V |
| DATA | amarillo (o blanco/azul) | el GPIO del bus |
| GND | negro | GND |

La **resistencia pull-up de 4,7 kΩ** entre DATA y 3,3 V no es opcional. La
línea de datos es *open-drain*: ningún dispositivo la conduce nunca a nivel
alto, solo la tira a bajo y la suelta. La resistencia es lo que devuelve la
línea a 3,3 V entre bits; sin ella, toda lectura es basura. Una resistencia
por bus, sin importar cuántas sondas haya.

:::tip
La configuración `onewire_bus` tiene un conmutador de **pull-up interno** que
usa la resistencia interna aproximada de 45 kΩ del ESP32. Puede funcionar con
una sola sonda y un cable corto, pero es mucho más débil que la resistencia de
4,7 kΩ recomendada: para cualquier cosa más larga que una protoboard, pon la
resistencia real.
:::

Las sondas vendidas "con adaptador/módulo" suelen traer ya la resistencia en la
placa: no añadas una segunda.

## Muchos dispositivos en un solo cable

![Topología del bus: ESP32 con una pull-up y tres sondas en una sola línea de datos, cada una con su propia dirección ROM](../../../../../assets/diagrams/onewire-bus.svg)

Las sondas nuevas se cablean **en paralelo** sobre las mismas tres líneas:
pincha DATA, 3,3 V y GND donde te convenga. Una cadena en serie (sonda a
sonda a lo largo de un cable) es eléctricamente la más limpia; también van
bien derivaciones cortas desde una línea principal. Tramos de varios metros son
normales con la pull-up de 4,7 kΩ.

## Direcciones: cómo evitan chocar las sondas

Cada dispositivo 1-Wire se identifica con una **dirección ROM de 64 bits**:

![Anatomía de una dirección ROM de 64 bits: código de familia de 8 bits, serie única de 48 bits, CRC de 8 bits; el escaneo descubre, la coincidencia dirige a un solo dispositivo](../../../../../assets/diagrams/onewire-rom.svg)

Dos operaciones hacen que el cable compartido funcione:

- **Search ("scan" en el portal)** - una eliminación binaria ingeniosa que
  descubre todas las direcciones del bus sin conocer ninguna de antemano.
  El dispositivo de bus de Gekko expone esto como comando **scan**; los
  resultados (código de familia, dirección, estado CRC) alimentan el diálogo
  de creación del DS18B20.
- **Match** - para hablar con un solo dispositivo, el controlador envía antes
  su dirección completa; solo ese dispositivo responde. Los dispositivos
  nunca hablan por su cuenta, así que no hay colisiones: el controlador
  siempre hace polling.

El primer byte es el **código de familia**: `28` significa "sensor de
temperatura DS18B20". El escaneo reporta todo lo que encuentra y Gekko filtra
los candidatos DS18B20 por ese código.

## Configuración

| Campo | Valor por defecto | Significado |
| --- | --- | --- |
| `gpioPin` | `4` | El pin de datos del bus |
| `internalPullup` | off | Usa la débil pull-up interna del ESP32 en lugar de una pull-up externa de 4,7 kΩ |
| `enabled` | on | Deshabilitar el bus bloquea todos los sensores que cuelgan de él (`dependency_blocked`) |

## Solución de problemas

- **El escaneo no encuentra nada** - primero revisa la pull-up y después el
  orden del cableado (invertir VDD y DATA es el error clásico; las sondas lo
  aguantan).
- **La sonda aparece con bandera CRC** - mal contacto o interferencia;
  acorta la línea, mejora las uniones y aléjala de cables de red y fuentes conmutadas.
- **Dos sondas, una sola dirección mostrada** - escaneaste antes de conectar
  la segunda sonda; vuelve a ejecutar el escaneo.
