---
title: Analogeingaenge & Multiplexer
description: Analoge Spannungen in Gekko lesen - der eigene ESP32-ADC, der praezise ADS1115 und der 16-Kanal-CD74HC4067-Multiplexer, alles hinter einem einzigen Kanaltyp.
sidebar:
  order: 11
---

## Warum Analogeingaenge?

Viele Sensoren fuer Aquarium und Gewaechshaus sprechen kein digitales Protokoll
- sie geben einfach eine **Spannung** aus, die sich mit dem Messwert aendert:
ein NTC-Thermistor, ein pH- oder ORP-Board, ein TDS-/EC-Stift, ein
Fotowiderstand, ein Bodenfeuchte-Pad, ein Druck- oder Wasserstandssensor. Um
sie zu lesen, muss irgendetwas diese Spannung in eine Zahl verwandeln - ein
**ADC** (Analog-Digital-Wandler).

Gekko trennt *woher die Spannung kommt* (die ADC-Hardware) von *was die Zahl
bedeutet* (Temperatur, pH, Pegel). Diese Seite behandelt die erste Haelfte -
die vier Geraetetypen, die einen Rohwert liefern. Sensoren, die diesen Wert
interpretieren, wie der
[NTC-Thermistor](/gekko/de/reference/devices/ntc-thermistor/), haengen von einem
dieser Geraete ab und liefern die Mathematik dazu.

Jedes Analogeingangs-Geraet meldet dasselbe: einen Wert in **Millivolt** (die
massgebliche Zahl) plus einen rohen ADC-Code fuer Diagnosen, mit Gueltigkeits-
Flag. Ein abgezogener oder ungueltiger Eingang erscheint als *invalid*, nie
als falsche Null.

## Die drei Wege zu einem Messwert

| Typ | Hardware | Kanaele | Haengt ab von |
| --- | --- | --- | --- |
| `analog_port_input` | Der eingebaute ADC des ESP32 | 1 (sein Pin) | - |
| `ads1115_hub` | ADS1115 16-Bit-I2C-ADC | 4 | einem [I2C-Bus](/gekko/de/reference/devices/i2c-bus/) |
| `cd74hc4067_hub` | CD74HC4067-Analog-Multiplexer | 16 | - (besitzt GPIO-Pins) |

Welcher passt:

- **`analog_port_input`** - die Option ohne Zusatzteile. Der ESP32 hat bereits
  einen ADC; hier wird einer seiner Pins direkt gelesen. Gut fuer einen groben
  Wert (Fotowiderstand, grober Füllstand), wo ±ein paar Prozent egal sind. Der
  On-Chip-ADC ist nur 12 Bit und leicht nichtlinear, und die Haelfte seiner
  Pins funktioniert nicht mehr, sobald WiFi an ist (siehe die Hinweise unten).
- **`ads1115_hub`** - wenn Genauigkeit zaehlt. Der ADS1115 ist ein 16-Bit-
  I2C-ADC mit programmierbarem Gain, also wird ein kleines Signal (z. B. ein
  pH-Board oder ein praeziser Thermistor) sauber und wiederholbar gemessen.
  Vier Kanaele pro Chip, und bis zu vier Chips auf einem Bus (Adressen
  `0x48`-`0x4B`).
- **`cd74hc4067_hub`** - wenn du *viele* guenstige Kanaele brauchst. Der
  CD74HC4067 ist ein 16-facher Analogschalter: Er verbindet einen von 16
  Eingaengen mit einem gemeinsamen Pin, den du in irgendeinen ADC fuehrst
  (standardmaessig den ESP32-ADC). Sechzehn Bodenfeuchte-Pads oder
  Wasserstandssensoren an einem ADC-Pin - aber sie teilen sich weiterhin die
  Genauigkeit des On-Chip-ADCs und werden nacheinander gelesen.

## Hubs und Kanaele

Der ESP32-Port-Eingang ist eigenstaendig - er *ist* genau ein Messwert, also
legst du ihn einfach an und zeigst einen Sensor darauf.

Die beiden Mehrkanal-Chips funktionieren anders und spiegeln das
[Portexpander-Muster](/gekko/de/guides/devices-and-dependencies/): Das **Hub**-
Geraet besitzt den Chip und seine Pins, und jeder wirklich genutzte Kanal ist
ein eigenes **`analog_input_channel`**-Geraet, das vom Hub abhaengt.

![Ein ADS1115-Hub mit vier Kanaelen; zwei Kanalgeraete haengen daran und werden von Sensoren gelesen, der Hub haengt von einem I2C-Bus ab](../../../../../assets/diagrams/analog-input-hub.svg)

Ein Zwei-Sonden-ADS1115-Setup besteht also aus drei Geraeten: dem
`ads1115_hub` und zwei `analog_input_channel`-Geraeten (Kanal 0 und 1), die
darauf zeigen. Jeder Kanal ist unabhaengig benannt, aktiviert und gepollt -
und jeder kann von seinem eigenen Sensor gelesen werden. Es gibt **einen**
Kanaltyp fuer beide Hub-Arten: Ein Kanal nennt nie den konkreten Chip, er
fragt den Hub einfach nach "Kanal N". Darum funktioniert ein
`analog_input_channel` gleich, egal ob sein Hub ein ADS1115 (Kanaele 0-3) oder
ein CD74HC4067 (Kanaele 0-15) ist. Das Erstellformular begrenzt die Kanalnummer
auf das, was der gewaehlte Hub tatsaechlich bietet.

Ein Kanalgeraet ist bewusst klein - es nennt nur seinen Hub und die
Kanalnummer, sampelt dann und meldet Millivolt:

![Einstellungen eines Analogeingangskanals: Hub-Auswahl, Kanalnummer, Oversampling und die Live-Spannung](../../../../../assets/screenshots/device-analog-input-channel.png)

Zwei Kanaele koennen nicht dieselbe Nummer auf einem Hub beanspruchen - Gekko
lehnt den zweiten ab, genauso wie zwei Portexpander-Schalter auf demselben Pin.

## ADS1115 einrichten

1. Erstelle einen **[I2C-Bus](/gekko/de/reference/devices/i2c-bus/)** auf deinen
   SDA/SCL-Pins (falls noch nicht vorhanden) und nutze **Scan bus**, um zu
   bestaetigen, dass der ADS1115 antwortet - meist bei `0x48`.
2. Erstelle einen **`ads1115_hub`**, waehle diesen Bus und setze Adresse und
   Gain.
3. Fuer jeden verdrahteten Eingang erstellst du einen
   **`analog_input_channel`**, waehlt den Hub und die Kanalnummer (0-3 = die
   A0-A3 des ADS1115).
4. Zeige einen Sensor darauf (oder beobachte einfach die Live-Millivolt des
   Kanals).

![ADS1115-Hub-Einstellungen: I2C-Bus, Adresse mit Scan, Gain und Datenrate](../../../../../assets/screenshots/device-ads1115-hub.png)

**Gain** bestimmt den Eingangsbereich und damit die Aufloesung. Waehle den
kleinsten Bereich, der dein Signal noch bequem abdeckt - ein kleinerer Bereich
verteilt die 16 Bit ueber weniger Volt, also wird jeder Schritt feiner:

| Gain | Vollbereich | Wann nutzen |
| --- | --- | --- |
| `fsr6144` | ±6.144 V | Unter 3.3 V nie noetig - clippt den Codebereich |
| `fsr4096` | ±4.096 V | Ein Signal, das die vollen 3.3 V erreichen kann |
| `fsr2048` | ±2.048 V | **Standard** - gut fuer die meisten 0-2 V-Signale |
| `fsr1024` | ±1.024 V | Kleine Signale unter ~1 V |
| `fsr0512` | ±0.512 V | |
| `fsr0256` | ±0.256 V | Sehr kleine Signale |

:::caution[Die Versorgung nicht ueberschreiten]
Der ADS1115 kann im Code bis zu ±6.144 V darstellen, aber du darfst einen
Kanal niemals mit mehr als der Chip-Versorgung (VDD, hier also 3.3 V) speisen.
Die Gain-Einstellung legt nur fest, wie der Codebereich auf Volt abgebildet
wird - sie schuetzt den Eingang nicht.
:::

## CD74HC4067-Multiplexer einrichten

Der CD74HC4067 braucht keinen Bus. Er hat vier **Adresspins** (S0-S3), die
Gekko ansteuert, um auszuwählen, welcher der 16 Eingaenge mit dem
gemeinsamen **SIG**-Pin verbunden wird, den du an einen ADC-Pin legst
(standardmaessig einen ESP32-ADC-Pin):

1. Verdrahte S0-S3 mit vier GPIOs, SIG mit einem ADC-faehigen Pin und optional
   EN mit einem GPIO (EN an GND legen, wenn du ihn nicht verdrahtest).
2. Erstelle einen **`cd74hc4067_hub`**, trage die vier Select-Pins, den SIG-Pin
   und seine Attenuation ein.
3. Erstelle pro Eingang einen **`analog_input_channel`**, waehle den Hub und
   Kanal 0-15.

![CD74HC4067-Hub-Einstellungen: die vier S0-S3-Select-Pins, Enable-Pin, Signal-Pin und dessen Attenuation](../../../../../assets/screenshots/device-cd74hc4067-hub.png)

Weil alle 16 Kanaele ueber einen einzigen ESP32-ADC-Pin laufen, teilen sie sich
die Genauigkeit dieses ADCs und die WiFi-Pin-Einschraenkung unten - der
Multiplexer bringt dir Kanalanzahl, nicht Praezision. Die Messungen laufen
nacheinander: Gekko setzt die Addressleitungen, wartet einen Tick, bis sich der
Mux beruhigt hat, und sampelt dann - viele Kanaele zu scannen ist also von
Natur aus getaktet statt instantan.

## Die ESP32-ADC-Fallen (Port-Eingang & CD74HC4067 SIG)

Sowohl `analog_port_input` als auch der SIG-Pin des CD74HC4067 nutzen den
On-Chip-ADC des ESP32, und der hat zwei Dinge, die man wissen sollte:

- **Mit WiFi ADC1-Pins verwenden.** GPIO **32-39** sind ADC1 und bleiben
  lauffaehig, waehrend WiFi laeuft; die ADC2-Pins nicht - WiFi belegt ADC2,
  also haengt eine Messung dort oder liefert Unsinn. GPIO **34-39** sind nur
  Eingaege (keine internen Pull-ups), genau richtig fuer einen Sensoreingang.
  Der Standard-Pin ist **34**.
- **Attenuation setzt den Eingangsbereich.** Der rohe ADC misst nur bis etwa
  1.1 V; die Attenuation skaliert hoehere Spannungen in dieses Fenster.
  Nutze den groessten Bereich (`11db`, Standard), ausser dein Signal ist
  wirklich klein:

  | Attenuation | Nutzbarer Eingangsbereich |
  | --- | --- |
  | `0db` | ~0 - 0.95 V |
  | `2_5db` | ~0 - 1.3 V |
  | `6db` | ~0 - 1.75 V |
  | `11db` | ~0 - 3.1 V (**Standard**, voller Bereich) |

Wenn die genaue Spannung wichtig ist, nimm lieber einen ADS1115 - der
On-Chip-ADC ist bequem, nicht praezise.

## Glattung und Reporting

Jeder Eingangs-Messwert ist der Mittelwert mehrerer unmittelbar nacheinander
aufgenommener ADC-Samples, was Rauschen schon vor dem Melden reduziert. Wie
oft gemessen und wie eifrig Updates gesendet werden, ist pro Geraet/Kanal
konfigurierbar - nach demselben Reporting-Delta-Prinzip wie bei den
Temperatursensoren, damit ein zitterndes Rohsignal WebSocket oder Historie
nicht flutet.

## Konfiguration

### `analog_port_input`

| Feld | Standard | Bedeutung |
| --- | --- | --- |
| `gpioPin` | `34` | Der zu lesende ADC-Pin (ADC1 verwenden: 32-39, mit WiFi an) |
| `attenuation` | `11db` | Eingangsbereich - siehe Tabelle oben |
| `adcSamples` | `8` | Pro Messwert gemittelte Samples (1-32) |
| `pollMs` | `1000` | Wie oft gelesen wird |
| `reportDeltaMilliVolts` | `10` | Mindestaenderung, bevor ein neuer Wert gesendet wird |
| `reportAlways` | aus | Jeden Poll senden, unabhaengig vom Delta |

### `ads1115_hub`

| Feld | Standard | Bedeutung |
| --- | --- | --- |
| `i2cAddress` | `0x48` | ADS1115-Adresse (`0x48`-`0x4B` per ADDR-Pin) |
| `gain` | `fsr2048` | Vollbereich / PGA - siehe Gain-Tabelle |
| `dataRateSps` | `128` | Samples pro Sekunde: `8`-`860`; hoeher = schneller, aber noisiger |

### `cd74hc4067_hub`

| Feld | Standard | Bedeutung |
| --- | --- | --- |
| `selectPins` | `[16, 17, 18, 19]` | Die vier S0-S3-Adress-GPIOs |
| `sigPin` | `34` | ADC-Pin, an den der gemeinsame SIG-Ausgang geht (ADC1 mit WiFi an) |
| `sigAttenuation` | `11db` | Eingangsbereich fuer diesen ADC-Pin - siehe Attenuation-Tabelle |
| `enablePin` | ungenutzt | Optionaler EN-GPIO; leer lassen und EN an GND legen |

### `analog_input_channel`

| Feld | Standard | Bedeutung |
| --- | --- | --- |
| `channel` | `0` | Welcher Hub-Kanal (0-3 bei ADS1115, 0-15 bei CD74HC4067) |
| `adcSamples` | `4` | Pro Messwert gemittelte Samples (1-32) |
| `pollMs` | `1000` | Wie oft gelesen wird |
| `reportDeltaMilliVolts` | `10` | Mindestaenderung, bevor ein neuer Wert gesendet wird |
| `reportAlways` | aus | Jeden Poll senden, unabhaengig vom Delta |

## Wohin der Messwert geht

Ein Analogeingang ist fuer sich nur eine Spannung auf dem Dashboard - nuetzlich
fuer einen schnellen Check, aber der eigentliche Zweck ist meistens ein
Sensor:

- ein **[NTC-Thermistor](/gekko/de/reference/devices/ntc-thermistor/)** macht aus
  dem Wert eine Temperatur, die dann ein
  [Thermostat](/gekko/de/reference/devices/thermostat/) steuern kann;
- die Millivolt erscheinen in
  [Display-Platzhaltern](/gekko/de/guides/displays/) fuer ein OLED/TFT;
- auf [MQTT-Builds](/gekko/de/guides/mqtt-home-assistant/) ist jeder Blatt-
  Eingang (Port-Eingang und jeder Kanal) in Home Assistant als `voltage`-
  Sensor auffindbar. Hubs selbst nicht - sie liefern Kanaele, keinen eigenen
  Messwert.

Firmware-Interna:
[`docs/analog-input.md`](https://github.com/yoreek/gekko/blob/master/docs/analog-input.md).
