---
title: Sensor de temperatura con termistor NTC
description: Leer temperatura con un termistor NTC barato en Gekko: el divisor de tensión, presets, las curvas Beta y Steinhart-Hart, y la calibración.
sidebar:
  order: 12
---

## ¿Qué es un termistor NTC?

Un termistor NTC es una resistencia cuya resistencia **baja cuando se
calienta** (NTC = Negative Temperature Coefficient). Son los sensores de
temperatura más baratos que existen, cuestan apenas unos céntimos, y vienen
en formatos de bolita de vidrio, epoxi e impermeables. La contrapartida frente
a un [DS18B20](/gekko/es/reference/devices/ds18b20/) es que un termistor es
*analógico*: solo cambia de resistencia, así que tienes que medir esa
resistencia y convertirla a temperatura. Gekko hace ambas cosas.

Frente a un DS18B20, un NTC es más barato y puede ser físicamente diminuto o
muy rápido de responder, pero es menos preciso de entrada, necesita una
resistencia y un ADC, y la resistencia del cable puede mover la lectura. Usa
un DS18B20 cuando quieras precisión plug-and-play; usa un NTC cuando quieras
algo barato, pequeño o rápido, o cuando ya tengas un
[ADS1115](/gekko/es/reference/devices/analog-inputs/) con un canal libre.

## Cableado: el divisor de tensión

No puedes leer resistencia directamente: lees un voltaje. Así que el
termistor va en serie con una **resistencia serie** fija para formar un
divisor entre la alimentación y tierra, y Gekko mide el voltaje en el punto
medio:

![Divisor de tensión NTC alimentando una entrada analógica, y luego el sensor NTC convirtiendo milivoltios en temperatura](../../../../../assets/diagrams/ntc-divider.svg)

Cuando la resistencia del NTC cambia con la temperatura, el voltaje del punto
medio se mueve; Gekko convierte ese voltaje de vuelta a la resistencia del
NTC (conoce la resistencia serie y la alimentación) y luego de resistencia a
temperatura. Una resistencia serie de **10 kΩ** combinada con un termistor de
**10 kΩ (a 25 °C)** es la combinación clásica y el valor por defecto de Gekko.

Ese punto medio es simplemente un voltaje analógico, así que el sensor NTC no
posee ningún pin ADC propio. Depende de una **[entrada analógica](/gekko/es/reference/devices/analog-inputs/)**,
lo que significa que puedes cablear el divisor a:

- el **ADC propio del ESP32** (`analog_port_input`) - lo más simple, lo menos preciso;
- un **canal ADS1115** (`analog_input_channel` sobre un `ads1115_hub`) - la
  opción precisa y la que hace realmente usable un termistor barato;
- un **canal CD74HC4067** - cuando muchos termistores comparten un pin ADC.

## Configurarlo

1. Crea la entrada analógica a la que está cableado el punto medio del
   divisor: consulta [entradas analógicas](/gekko/es/reference/devices/analog-inputs/).
   Un canal ADS1115 es la opción recomendada para una lectura estable.
2. Crea un **`ntc_thermistor_temperature_sensor`** y selecciona esa entrada
   analógica como dependencia.
3. Elige un **preset** que coincida con tu termistor o introduce los números
   a mano.

![Ajustes del sensor NTC: selector de entrada analógica, preset, valores del divisor, modo de fórmula y reporte](../../../../../assets/screenshots/device-ntc-thermistor.png)

### Los presets solo son un atajo

El formulario ofrece algunos modelos comunes de termistor:

| Preset | R serie | R nominal (25 °C) | Beta |
| --- | --- | --- | --- |
| Generic 10k B3950 | 10 kΩ | 10 kΩ | 3950 |
| EPCOS/TDK 10k B3435 | 10 kΩ | 10 kΩ | 3435 |
| Vishay 10k B3977 | 10 kΩ | 10 kΩ | 3977 |
| Semitec 100k B4267 | 100 kΩ | 100 kΩ | 4267 |

Un preset solo **rellena previamente los campos numéricos**: nada de esa
elección se guarda en el dispositivo. Seleccionar uno y luego retocar un valor
siempre es seguro: el preset nunca "pelea" con tus cambios. Elige el más
parecido y ajusta, o selecciona *Custom* e introduce los valores del datasheet
de tu termistor.

## Las dos curvas: Beta vs Steinhart-Hart

Convertir resistencia en temperatura requiere un modelo de la curva del
termistor. Gekko ofrece ambos estándares:

- **Ecuación Beta** - `1/T = 1/T₀ + (1/β)·ln(R/R₀)`. La forma de dos puntos
  que publica cualquier datasheet: resistencia nominal R₀ a temperatura
  nominal T₀ (normalmente 10 kΩ a 25 °C) más un único coeficiente **Beta**.
  Precisión aproximada de ±0,5-1 °C en un rango de acuario: más que suficiente
  para un calentador o un enfriador. Es la opción por defecto y la más fácil
  de rellenar.
- **Ecuación Steinhart-Hart** - `1/T = A + B·ln(R) + C·ln(R)³`. Tres
  coeficientes en vez de uno, más precisa en un rango más amplio cuando los
  conoces (o los ajustas a partir de una tabla resistencia/temperatura de 3
  puntos). Elígela solo si tienes los valores A/B/C; de lo contrario, Beta es
  la elección correcta.

Puedes cambiar entre ambas con el selector **formula mode**; el formulario
muestra los campos que necesita la ecuación elegida.

## Calibración y suavizado

Como la lectura de un termistor depende de tolerancias (del propio termistor,
de la resistencia serie, del ADC), el sensor usa el acondicionamiento estándar
de Gekko:

- un **offset/factor de calibración** para ajustar un error conocido frente a
  un termómetro de referencia;
- un **peso de suavizado** para amortiguar el último poco de jitter del ADC.

Pon un termómetro de referencia junto a la sonda, compara ambas lecturas y
ajusta el offset hasta que coincidan: ese ajuste de un solo punto elimina la
mayor parte del error de un termistor barato.

## Verlo

El sensor reporta su temperatura con una bandera de validez: si su entrada
analógica pasa a ser inválida (un divisor desconectado, un bus I2C enfermo
detrás de un ADS1115), la lectura aparece como *invalid*, nunca como un valor
antiguo o inventado. Haz clic en su tarjeta del panel para ver el valor en
vivo y la gráfica histórica, igual que con el DS18B20.

La temperatura alimenta todo lo demás en Gekko del mismo modo que cualquier
sensor de temperatura:

- un [termostato](/gekko/es/reference/devices/thermostat/) que gobierna un
  calentador o un enfriador;
- [marcadores de pantalla](/gekko/es/guides/displays/) en un OLED/TFT;
- Home Assistant como entidad `sensor` de solo lectura en
  [compilaciones MQTT](/gekko/es/guides/mqtt-home-assistant/).

## Configuración

| Campo | Valor por defecto | Significado |
| --- | --- | --- |
| `formulaMode` | `beta` | `beta` o `steinhart_hart` |
| `seriesResistorOhms` | `10000` | La resistencia fija del divisor, en ohmios |
| `supplyMilliVolts` | `3300` | Tensión de alimentación del divisor (rail de 3,3 V) |
| `nominalResistanceOhms` | `10000` | Resistencia del termistor a la temperatura nominal (R₀) |
| `nominalTempCelsius` | `25` | Temperatura nominal (T₀) |
| `betaCoefficient` | `3950` | Valor Beta (modo Beta) |
| `steinhartA` / `steinhartB` / `steinhartC` | `0` | Coeficientes Steinhart-Hart (modo Steinhart-Hart) |
| `unit` | `celsius` | Unidad de visualización |
| `pollMs` | `5000` | Cada cuánto leer |
| `reportDeltaCelsius` | `0.1` | Cambio mínimo antes de enviar una nueva lectura |
| `reportAlways` | off | Enviar en cada lectura sin importar el delta |

La temperatura cambia despacio: el sondeo por defecto de 5 s con un delta de
reporte pequeño mantiene tranquilas la WebSocket y el historial sin perder
nada real.

Internos del firmware, matemática de curvas y la tabla de presets:
[`docs/analog-input.md`](https://github.com/yoreek/gekko/blob/master/docs/analog-input.md).
