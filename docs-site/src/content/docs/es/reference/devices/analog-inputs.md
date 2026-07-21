---
title: Entradas analógicas y multiplexores
description: "Leer voltajes analógicos en Gekko: el ADC propio del ESP32, el preciso ADS1115 y el multiplexor CD74HC4067 de 16 canales, todo detrás de un único tipo de canal."
sidebar:
  order: 11
---

## ¿Por qué entradas analógicas?

Muchos sensores para acuario e invernadero no hablan un protocolo digital:
simplemente entregan un **voltaje** que cambia con lo que miden: un termistor
NTC, una placa de pH u ORP, una sonda TDS/EC, una fotorresistencia, una
almohadilla de humedad del suelo, un sensor de presión o de nivel de agua.
Para leer cualquiera de ellos, algo tiene que convertir ese voltaje en un
número: eso es un **ADC** (convertidor analógico-digital).

Gekko separa *de dónde viene el voltaje* (el hardware ADC) de *qué significa
el número* (temperatura, pH, nivel). Esta página trata la primera mitad: los
cuatro tipos de dispositivo que producen una lectura de voltaje en bruto. Los
sensores que interpretan esa lectura, como el
[termistor NTC](/gekko/es/reference/devices/ntc-thermistor/), dependen de uno de
estos y añaden la matemática.

Cada dispositivo de entrada analógica reporta lo mismo: una lectura en
**milivoltios** (el valor autoritativo) más un código ADC bruto para
diagnóstico, con una bandera de validez. Una entrada desconectada o inválida
aparece como *invalid*, nunca como un cero falso.

## Las tres formas de obtener una lectura

| Tipo | Hardware | Canales | Depende de |
| --- | --- | --- | --- |
| `analog_port_input` | El ADC integrado del propio ESP32 | 1 (su pin) | - |
| `ads1115_hub` | ADC I2C ADS1115 de 16 bits | 4 | un [bus I2C](/gekko/es/reference/devices/i2c-bus/) |
| `cd74hc4067_hub` | Multiplexor analógico CD74HC4067 | 16 | - (posee pines GPIO) |

Cuál conviene:

- **`analog_port_input`** - la opción sin piezas extra. El ESP32 ya tiene un
  ADC; esto lee uno de sus pines directamente. Vale para una lectura
  aproximada (una fotorresistencia, un flotador grosero) donde ±unos pocos
  puntos porcentuales no importan. El ADC integrado es solo de 12 bits y
  algo no lineal, y la mitad de sus pines dejan de funcionar en cuanto
  WiFi está activo (ver las advertencias más abajo).
- **`ads1115_hub`** - cuando importa la precisión. El ADS1115 es un ADC I2C
  de 16 bits con amplificador de ganancia programable, así que una señal
  pequeña (la salida de una placa de pH, un termistor preciso) se mide con
  limpieza y repetibilidad. Cuatro canales por chip, y hasta cuatro chips en
  un bus (direcciones `0x48`-`0x4B`).
- **`cd74hc4067_hub`** - cuando necesitas *muchos* canales baratos. El
  CD74HC4067 es un conmutador analógico de 16 vías: conecta una de 16 entradas
  a un pin compartido, que alimentas con cualquier ADC (por defecto, el ADC del
  ESP32). Dieciséis sondas de humedad del suelo o niveles de flotador en un
  solo pin ADC, pero siguen compartiendo la precisión del ADC integrado y se
  leen de una en una.

## Hubs y canales

La entrada de puerto del ESP32 es autónoma: *ella misma* es una lectura, así
que simplemente la creas y apuntas un sensor hacia ella.

Los dos chips multicanal funcionan distinto y reflejan el
[patrón de expansores de puerto](/gekko/es/guides/devices-and-dependencies/): el
dispositivo **hub** posee el chip y sus pines, y cada canal que realmente usas
es un dispositivo separado **`analog_input_channel`** que depende del hub.

![Un hub ADS1115 con cuatro canales; dos dispositivos de canal dependen de él y son leídos por sensores, el hub depende de un bus I2C](../../../../../assets/diagrams/analog-input-hub.svg)

Así que un montaje ADS1115 de dos sondas son tres dispositivos: el
`ads1115_hub` y dos dispositivos `analog_input_channel` (canal 0 y canal 1)
apuntando a él. Cada canal se nombra, habilita y sondea de forma
independiente, y cada uno puede ser leído por su propio sensor. Hay **un**
tipo de canal para ambos tipos de hub: un canal nunca nombra el chip concreto,
solo le pide al hub "canal N", así que un `analog_input_channel` funciona igual
si su hub es un ADS1115 (canales 0-3) o un CD74HC4067 (canales 0-15). El
formulario de creación limita el número de canal a lo que realmente ofrece el
hub seleccionado.

Un dispositivo de canal es deliberadamente pequeño: solo nombra su hub y su
número de canal, luego toma muestras y reporta milivoltios:

![Ajustes de un canal de entrada analógica: selector de hub, número de canal, oversampling y el voltaje en vivo](../../../../../assets/screenshots/device-analog-input-channel.png)

Dos canales no pueden reclamar el mismo número sobre un mismo hub: Gekko
rechaza el segundo, igual que rechaza dos interruptores de expansor de puerto
en el mismo pin.

## Configurar un ADS1115

1. Crea un **[bus I2C](/gekko/es/reference/devices/i2c-bus/)** en tus pines SDA/
   SCL (si no tienes uno) y usa **Scan bus** para confirmar que el ADS1115
   responde, normalmente en `0x48`.
2. Crea un **`ads1115_hub`**, selecciona ese bus y fija su dirección y ganancia.
3. Para cada entrada cableada, crea un **`analog_input_channel`**, selecciona
   el hub y el número de canal (0-3 = A0-A3 del ADS1115).
4. Apunta un sensor (o simplemente observa los milivoltios vivos del canal) a
   cada canal.

![Ajustes del hub ADS1115: bus I2C, dirección con escaneo, ganancia y tasa de datos](../../../../../assets/screenshots/device-ads1115-hub.png)

**Gain** define el rango de entrada y, por tanto, la resolución. Elige el
rango más pequeño que cubra cómodamente tu señal: un rango menor reparte los
16 bits sobre menos voltios, así que cada paso es más fino:

| Gain | Rango completo | Cuándo usarlo |
| --- | --- | --- |
| `fsr6144` | ±6,144 V | Nunca hace falta a 3,3 V: recorta el rango de código |
| `fsr4096` | ±4,096 V | Una señal que puede llegar a todo el rail de 3,3 V |
| `fsr2048` | ±2,048 V | **Por defecto** - buena para la mayoría de señales 0-2 V |
| `fsr1024` | ±1,024 V | Señales pequeñas por debajo de ~1 V |
| `fsr0512` | ±0,512 V | |
| `fsr0256` | ±0,256 V | Señales muy pequeñas |

:::caution[No superes la alimentación]
El ADS1115 puede *representar* hasta ±6,144 V en código, pero nunca debes
inyectar en un canal más tensión que la alimentación del chip (VDD, aquí 3,3 V).
El ajuste de ganancia solo decide cómo se mapea el rango de código a voltios:
no protege la entrada.
:::

## Configurar un multiplexor CD74HC4067

El CD74HC4067 no necesita bus. Tiene cuatro **pines de dirección** (S0-S3)
que Gekko conduce para elegir cuál de las 16 entradas se conecta al pin
compartido **SIG**, que cableas a un pin ADC (por defecto, un pin ADC del
ESP32):

1. Cablea S0-S3 a cuatro GPIO, SIG a un pin capaz de ADC y, opcionalmente, EN
   a un GPIO (si no cableas EN, llévalo a GND).
2. Crea un **`cd74hc4067_hub`**, introduce los cuatro pines de selección, el
   pin SIG y su atenuación.
3. Crea un **`analog_input_channel`** por entrada, seleccionando el hub y el
   canal 0-15.

![Ajustes del hub CD74HC4067: los cuatro pines S0-S3, el pin enable, el pin de señal y su atenuación](../../../../../assets/screenshots/device-cd74hc4067-hub.png)

Como los 16 canales pasan por un solo pin ADC del ESP32, comparten la
precisión de ese ADC y la restricción de pines con WiFi de abajo: el
multiplexor te da cantidad de canales, no precisión. Las lecturas son
secuenciales: Gekko cambia las líneas de dirección, espera un tick para que el
mux se asiente y luego muestrea; escanear muchos canales va, por tanto, a un
ritmo natural y no instantáneo.

## Las advertencias del ADC del ESP32 (entrada de puerto y SIG del CD74HC4067)

Tanto `analog_port_input` como el pin SIG del CD74HC4067 usan el ADC integrado
del ESP32, y hay dos cosas importantes que saber:

- **Usa pines ADC1 con WiFi.** GPIO **32-39** son ADC1 y siguen funcionando
  mientras WiFi está activo; los pines ADC2 no: WiFi se adueña de ADC2, así
  que una lectura allí se atasca o devuelve basura. GPIO **34-39** son solo
  de entrada (sin pull-ups internas), exactamente lo que quieres para un
  sensor. El pin por defecto es **34**.
- **La atenuación fija el rango de entrada.** El ADC bruto solo mide hasta
  ~1,1 V; la atenuación escala tensiones mayores hacia esa ventana. Usa el
  rango más amplio (`11db`, el valor por defecto) salvo que tu señal sea
  realmente pequeña:

  | Atenuación | Rango de entrada utilizable |
  | --- | --- |
  | `0db` | ~0 - 0,95 V |
  | `2_5db` | ~0 - 1,3 V |
  | `6db` | ~0 - 1,75 V |
  | `11db` | ~0 - 3,1 V (**por defecto**, rango completo) |

Para cualquier caso donde importe el voltaje exacto, prefiere un ADS1115: el
ADC integrado es cómodo, no preciso.

## Suavizado y reporte

Cada lectura de entrada es la media de varias muestras ADC tomadas una tras
otra, lo que reduce el ruido antes de que el valor se reporte. Cada cuánto se
muestra y con qué ganas se envían actualizaciones es configurable por
dispositivo/canal, siguiendo la misma idea de delta de reporte que usan los
sensores de temperatura, para que una señal ruidosa no inunde la WebSocket ni
las gráficas históricas.

## Configuración

### `analog_port_input`

| Campo | Valor por defecto | Significado |
| --- | --- | --- |
| `gpioPin` | `34` | El pin ADC a leer (usa ADC1: 32-39, con WiFi activo) |
| `attenuation` | `11db` | Rango de entrada: consulta la tabla de arriba |
| `adcSamples` | `8` | Muestras promediadas por lectura (1-32) |
| `pollMs` | `1000` | Cada cuánto leer |
| `reportDeltaMilliVolts` | `10` | Cambio mínimo antes de enviar una nueva lectura |
| `reportAlways` | off | Enviar en cada lectura sin importar el delta |

### `ads1115_hub`

| Campo | Valor por defecto | Significado |
| --- | --- | --- |
| `i2cAddress` | `0x48` | Dirección del ADS1115 (`0x48`-`0x4B` por el pin ADDR) |
| `gain` | `fsr2048` | Rango completo / PGA: mira la tabla de ganancia |
| `dataRateSps` | `128` | Muestras por segundo: `8`-`860`; más alto = más rápido pero más ruidoso |

### `cd74hc4067_hub`

| Campo | Valor por defecto | Significado |
| --- | --- | --- |
| `selectPins` | `[16, 17, 18, 19]` | Los cuatro GPIO de dirección S0-S3 |
| `sigPin` | `34` | Pin ADC al que sale el SIG compartido (ADC1 con WiFi activo) |
| `sigAttenuation` | `11db` | Rango de entrada para ese pin ADC: consulta la tabla de atenuación |
| `enablePin` | sin usar | GPIO EN opcional; déjalo sin definir y lleva EN a GND |

### `analog_input_channel`

| Campo | Valor por defecto | Significado |
| --- | --- | --- |
| `channel` | `0` | Qué canal del hub (0-3 en ADS1115, 0-15 en CD74HC4067) |
| `adcSamples` | `4` | Muestras promediadas por lectura (1-32) |
| `pollMs` | `1000` | Cada cuánto leer |
| `reportDeltaMilliVolts` | `10` | Cambio mínimo antes de enviar una nueva lectura |
| `reportAlways` | off | Enviar en cada lectura sin importar el delta |

## A dónde va la lectura

Una entrada analógica por sí sola es solo un voltaje en el panel: útil para
una comprobación rápida, pero normalmente la idea es alimentar un sensor:

- un **[termistor NTC](/gekko/es/reference/devices/ntc-thermistor/)** convierte
  la lectura en temperatura, que luego puede gobernar un
  [termostato](/gekko/es/reference/devices/thermostat/);
- los milivoltios aparecen en
  [marcadores de pantalla](/gekko/es/guides/displays/) para un OLED/TFT;
- en [compilaciones MQTT](/gekko/es/guides/mqtt-home-assistant/) cada entrada
  hoja (la entrada de puerto y cada canal) se descubre en Home Assistant como
  un sensor `voltage`. Los hubs no: ellos proporcionan canales, no una lectura
  propia.

Internales del firmware:
[`docs/analog-input.md`](https://github.com/yoreek/gekko/blob/master/docs/analog-input.md).
