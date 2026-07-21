---
title: I2C-Bus
description: Wie der I2C-Bus funktioniert - zwei Draehte, 7-Bit-Adressen, was in Gekko darauf laeuft und die eingebaute Busdiagnose.
sidebar:
  order: 4
  label: I2C-Bus
---

## Was ist I2C?

I2C (Inter-Integrated Circuit, "i-squared-C") ist der Arbeitspferd-Zweidraht-
Bus der Hobbyelektronik: eine **Daten**leitung (SDA) und eine
**Clock**-/Taktleitung (SCL), die sich alle Geraete teilen. Wie bei
[1-Wire](/gekko/de/reference/devices/onewire-bus/) koennen viele Geraete dieselben
Leitungen nutzen - hier hat aber jeder Chip eine kurze **7-Bit-Adresse**, die
meist im Datenblatt steht (und oft per Loetbruecken gewaehlt werden kann).

In Gekko ist der Bus selbst ein Geraet, `i2c_bus`: Es besitzt die beiden Pins,
fuehrt den Adress-Scan aus, und jedes I2C-Peripheriegaet, das du hinzufuegst,
deklariert eine Abhaengigkeit darauf.

## Verdrahtung

![I2C-Verdrahtung: SDA und SCL mit Pull-ups, OLED, HTU21, DS3231 und PCF8574 parallel, jedes mit seiner Adresse](../../../../../assets/diagrams/i2c-wiring.svg)

Beide Leitungen sind Open-Drain und brauchen Pull-up-Widerstaende auf 3,3 V.
In der Praxis musst du sie meist nicht selbst setzen: **Beinahe jedes
Breakout-Modul (OLED, RTC, HTU21, Expander) hat bereits Pull-ups an Bord**,
und das `i2c_bus`-Geraet aktiviert standardmaessig die internen Pull-ups des
ESP32. Nur ein nackter Chip an einer langen Leitung braucht explizite
Widerstaende (2,2-10 kΩ).

Geraete werden parallel angeschlossen: SDA an SDA, SCL an SCL, plus 3,3 V und
GND. Halte die Leitungen halbwegs kurz (zig Zentimeter bei Standardtempo) -
I2C ist ein Board-Bus, kein Kabel-Bus wie 1-Wire.

## Wer auf dem I2C-Bus in Gekko lebt

| Geraet | Typische Adresse |
| --- | --- |
| SSD1306-OLED-Display | `0x3C` (manchmal `0x3D`) |
| HTU21 Temperatur + Luftfeuchte | `0x40` |
| DS3231-Echtzeituhr | `0x68` |
| PCF8574 / PCF8575-Portexpander | `0x20`-`0x27` (per Jumper waehlbar) |

Jedes davon wird als eigenes Geraet erzeugt, das vom Bus abhaengt, mit seiner
Adresse in seiner eigenen Config. Zwei identische Chips (z. B. zwei PCF8574
auf verschiedenen Jumper-Adressen) sind einfach zwei Geraete auf demselben
Bus - Gekko lehnt es ab, zwei Geraete mit derselben Adresse auf einem Bus zu
erstellen.

## Scan und Diagnose

Die Geraeteseite hat einen **Scan bus**-Button - er prueft alle gueltigen
Adressen und listet alles auf, was antwortet. Das ist der schnellste Weg, um
Verdrahtung und reale Adresse eines Moduls zu bestaetigen. Darunter liegen die
**Bus-Diagnosen**: fortlaufende Fehlerzaehler, der letzte Fehlercode und der
Transaktionszustand, plus ein Reset-Button. Ein Verdrahtungsproblem zeigt sich
hier zuerst - Sensoren auf einem kranken Bus melden `dependency_blocked`
statt falscher Werte.

![I2C-Bus-Einstellungen mit Scan und Diagnose](../../../../../assets/screenshots/device-i2c-bus.png)

## Konfiguration

| Feld | Standard | Bedeutung |
| --- | --- | --- |
| `sdaPin` | `21` | Datenleitung (ESP32s uebliche I2C-Pins sind 21/22) |
| `sclPin` | `22` | Taktleitung |
| `frequencyHz` | `100000` | Bustakt, 1-400 000 Hz; 100 kHz ist der sichere Standard, 400 kHz funktioniert mit kurzen Leitungen |
| `internalPullup` | an | Die internen Pull-ups des ESP32 verwenden (zusammen mit Modul-Pull-ups unproblematisch) |
| `enabled` | an | Das Deaktivieren des Busses blockiert jedes Geraet darauf |

## Fehlerbehebung

- **Scan findet nichts** - vertauschte SDA/SCL sind der Klassiker; pruefe auch
  3,3 V und GND am Modul.
- **Geraet bei anderer Adresse gefunden** - Jumper (Expander) oder eine
  `0x3D`-OLED-Variante; benutze die gescannte Adresse.
- **Fehler unter Last / bei langen Leitungen** - `frequencyHz` auf 100 kHz
  senken, Leitungen kuerzen und Displaykabel von Relais-/Netzleitungen fernhalten.
