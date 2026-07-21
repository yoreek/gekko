---
title: Sensore di temperatura a termistore NTC
description: Lettura della temperatura da un economico termistore NTC in Gekko — il partitore di tensione, i preset, le curve Beta e Steinhart-Hart e la calibrazione.
sidebar:
  order: 12
---

## Cos'è un termistore NTC?

Un termistore NTC è una resistenza la cui resistenza **diminuisce quando si
scalda** (NTC = Negative Temperature Coefficient). Sono il sensore di
temperatura più economico che ci sia — pochi centesimi — e arrivano in forma di
glassbead, epoxy e sonde impermeabili. Il rovescio rispetto a un
[DS18B20](/gekko/it/reference/devices/ds18b20/) è che un termistore è
*analogico*: cambia solo resistenza, quindi devi misurare quella resistenza e
convertirla in temperatura. Gekko fa entrambe le cose.

Rispetto a un DS18B20, un NTC è più economico e può essere fisicamente
piccolo o molto veloce a rispondere, ma è meno accurato out of the box, ha
bisogno di una resistenza e di un ADC, e la resistenza del cavo può spostare
la lettura. Usa un DS18B20 quando vuoi precisione plug-and-play; usa un NTC
quando vuoi qualcosa di economico, piccolo o veloce — o quando hai già un
[ADS1115](/gekko/it/reference/devices/analog-inputs/) con un canale libero.

## Cablaggio: il partitore di tensione

Non puoi leggere la resistenza direttamente — leggi una tensione. Quindi il
termistore va in serie con una **resistenza serie** fissa per formare un
partitore tra alimentazione e massa, e Gekko misura la tensione al punto medio:

![NTC voltage divider feeding an analog input, then the NTC sensor converting millivolts to a temperature](../../../../../assets/diagrams/ntc-divider.svg)

Quando la resistenza NTC cambia con la temperatura, cambia la tensione al
centro; Gekko converte quella tensione di nuovo nella resistenza dell'NTC
(conosce la resistenza serie e l'alimentazione), poi dalla resistenza alla
temperatura. Una resistenza serie da **10 kΩ** abbinata a un termistore da
**10 kΩ (a 25 °C)** è la combinazione classica e il default di Gekko.

Quel punto medio è solo una tensione analogica — quindi il sensore NTC non
possiede da solo un pin ADC. Dipende da un
**[ingresso analogico](/gekko/it/reference/devices/analog-inputs/)**, il che
significa che puoi collegare il partitore a:

- il **pin ADC nativo dell'ESP32** (`analog_port_input`) — il più semplice,
  il meno preciso;
- un **canale ADS1115** (`analog_input_channel` su `ads1115_hub`) — l'opzione
  precisa, e quella che rende davvero usabile un termistore economico;
- un **canale CD74HC4067** — quando hai molti termistori che condividono un
  solo pin ADC.

## Configurazione

1. Crea l'ingresso analogico a cui è cablato il punto medio del partitore —
   vedi [Ingressi analogici](/gekko/it/reference/devices/analog-inputs/). Un
   canale ADS1115 è la scelta consigliata per una lettura stabile.
2. Crea un **`ntc_thermistor_temperature_sensor`** e seleziona quell'ingresso
   analogico come dipendenza.
3. Scegli un **preset** che corrisponde al tuo termistore, oppure inserisci i
   valori a mano.

![NTC sensor settings: analog input picker, preset, divider values, formula mode and reporting](../../../../../assets/screenshots/device-ntc-thermistor.png)

### I preset sono solo una scorciatoia

Il form offre alcuni modelli comuni di termistore:

| Preset | Series R | Nominal R (25 °C) | Beta |
| --- | --- | --- | --- |
| Generic 10k B3950 | 10 kΩ | 10 kΩ | 3950 |
| EPCOS/TDK 10k B3435 | 10 kΩ | 10 kΩ | 3435 |
| Vishay 10k B3977 | 10 kΩ | 10 kΩ | 3977 |
| Semitec 100k B4267 | 100 kΩ | 100 kΩ | 4267 |

Un preset **precompila solo i campi numerici** — niente della scelta viene
salvato sul dispositivo. Selezionarne uno e poi modificare un valore dopo è
sempre sicuro; il preset non "litiga" mai con le tue modifiche. Scegli quello
più vicino e regola, oppure seleziona *Custom* e inserisci i valori del
datasheet del tuo termistore.

## Le due curve: Beta vs Steinhart-Hart

Convertire resistenza in temperatura richiede un modello della curva del
termistore. Gekko offre entrambi gli standard:

- **Equazione Beta** — `1/T = 1/T₀ + (1/β)·ln(R/R₀)`. La forma a due punti che
  ogni datasheet pubblica: resistenza nominale R₀ alla temperatura nominale T₀
  (di solito 10 kΩ a 25 °C) più un singolo coefficiente **Beta**. Precisa a
  circa ±0,5–1 °C in un range da acquario — ampiamente sufficiente per un
  heater o un chiller. È il default e il più semplice da compilare.
- **Equazione Steinhart-Hart** — `1/T = A + B·ln(R) + C·ln(R)³`. Tre
  coefficienti invece di uno, più precisa su un range più ampio quando li
  conosci (o li adatti da una tabella resistenza/temperatura a 3 punti).
  Sceglila solo se hai i valori A/B/C; altrimenti Beta è la scelta giusta.

Passa tra le due con il selettore **formula mode**; il form mostra i campi
richiesti dall'equazione scelta.

## Calibrazione e smoothing

Poiché una lettura di termistore dipende da tolleranze (del termistore stesso,
della resistenza serie, dell'ADC), il sensore sfrutta il condizionamento
standard di Gekko:

- un **offset/factor di calibrazione** per correggere un errore noto rispetto a
  un termometro di riferimento;
- un **peso di smoothing** per smorzare l'ultimo jitter dell'ADC.

Metti un termometro di riferimento accanto alla sonda, leggi entrambi e
ritocca l'offset finché coincidono — quel trim a punto singolo rimuove la
maggior parte dell'errore di un termistore economico.

## Monitoraggio

Il sensore riporta la temperatura con un flag di validità — se il suo ingresso
analogico diventa invalido (partitore scollegato, bus I2C malato dietro un
ADS1115), la lettura appare come *invalid*, mai come un numero vecchio o
finto. Clicca sulla sua tile in dashboard per il valore live e un grafico
storico, esattamente come per il DS18B20.

La temperatura alimenta tutto il resto in Gekko allo stesso modo di qualsiasi
altro sensore di temperatura:

- un [thermostat](/gekko/it/reference/devices/thermostat/) che pilota un
  riscaldatore o un chiller;
- [display placeholders](/gekko/it/guides/displays/) su un OLED/TFT;
- Home Assistant come entità `sensor` sola lettura nelle
  [build MQTT](/gekko/it/guides/mqtt-home-assistant/).

## Configurazione

| Field | Default | Meaning |
| --- | --- | --- |
| `formulaMode` | `beta` | `beta` o `steinhart_hart` |
| `seriesResistorOhms` | `10000` | La resistenza fissa del partitore, in ohm |
| `supplyMilliVolts` | `3300` | Tensione di alimentazione del partitore (rail 3,3 V) |
| `nominalResistanceOhms` | `10000` | Resistenza del termistore alla temperatura nominale (R₀) |
| `nominalTempCelsius` | `25` | La temperatura nominale (T₀) |
| `betaCoefficient` | `3950` | Valore Beta (modalità Beta) |
| `steinhartA` / `steinhartB` / `steinhartC` | `0` | Coefficienti Steinhart-Hart (modalità Steinhart-Hart) |
| `unit` | `celsius` | Unità di visualizzazione |
| `pollMs` | `5000` | Ogni quanto leggere |
| `reportDeltaCelsius` | `0.1` | Variazione minima prima di inviare una nuova lettura |
| `reportAlways` | off | Invia ogni poll indipendentemente dal delta |

La temperatura cambia lentamente — il poll predefinito di 5 s con un piccolo
delta di report mantiene WebSocket e storico tranquilli senza perdere niente
di reale.

Firmware internals, curve maths, and the preset table:
[`docs/analog-input.md`](https://github.com/yoreek/gekko/blob/master/docs/analog-input.md).
