---
title: Display e designer di layout
description: Configura display a pixel, a caratteri e a sette segmenti con il designer visuale di Gekko.
sidebar:
  order: 3
---

Gekko supporta cinque tipi di display tramite un **designer visuale di layout**
condiviso. Configuri pagine e widget nel browser con anteprima; coordinate e
widget si adattano al display.

| Tipo | Hardware | Coordinate | Widget |
| --- | --- | --- | --- |
| `ssd1306` | OLED I2C monocromatico | Pixel | Testo, forme e bitmap |
| `st7735` | TFT SPI a colori | Pixel | Testo, forme e bitmap RGB565 |
| `lcd1602` | HD44780 16 × 2 tramite PCF857x | Celle carattere | Character |
| `lcd2004` | HD44780 20 × 4 tramite PCF857x | Celle carattere | Character |
| `tm1637` | Modulo a sette segmenti a quattro cifre | Posizioni cifra | Digital |

## Configurare un display

1. Crea prima i dispositivi necessari:
   - un [**bus I2C**](/gekko/it/reference/devices/i2c-bus/) per SSD1306;
   - un [**bus SPI**](/gekko/it/reference/devices/spi-bus/) per ST7735;
   - un [**espansore di porte**](/gekko/it/reference/devices/port-expanders/)
     PCF8574/PCF8575 per LCD1602/LCD2004;
   - due dispositivi `gpio_switch` per CLK e DIO del TM1637.
2. Crea il display, seleziona questi dispositivi come dipendenze e configura
   indirizzo, cablaggio, pin di controllo, luminosità o rotazione.
3. Apri il dispositivo e fai clic su **Design** per entrare nel designer di
   layout.

![Display layout designer](../../../../assets/screenshots/portal-display-designer.png)

## Pagine e widget

Un layout contiene **pagine** con **widget** posizionati. I display a pixel
usano pixel, LCD1602/LCD2004 usano celle carattere e TM1637 usa posizioni
cifra. Il designer consente solo i widget compatibili e mostra un'anteprima
adeguata. I layout vengono salvati sul dispositivo e inclusi nei
[bundle di backup](/gekko/it/guides/backup-restore/).

## Valori live: placeholder di metrica

I widget Text, Character e Digital possono mescolare testo statico con
**placeholder** risolti al momento del render.

![Widget text with placeholders on the left, the rendered OLED output with live values on the right](../../../../assets/diagrams/display-placeholders.svg)

Non devi memorizzare la sintassi — il **placeholder builder** del designer
elenca ogni metrica disponibile di ogni dispositivo con un valore di preview
live, e inserisce il placeholder per te. I placeholder tipizzati vengono
validati a ogni battuta. Alcuni esempi:

```text
Room {{dev.670845748.temperature | fixed:1}}
IP {{system.wifi.station_ip}}
Now {{system.time | format:HH:mm}}
Up {{system.uptime}}
```

Forme dei placeholder:

- `{{dev.<deviceId>.<metricKey>}}` — una metrica da qualsiasi dispositivo
  (temperatura, stato, …). Il designer ha un builder che elenca tutto ciò che
  è disponibile.
- `{{system.<metricKey>}}` — metriche di sistema come `time` (orologio) e
  `uptime` (tempo trascorso dall'avvio).
- `{{system.wifi.<metricKey>}}` — metriche WiFi come `station_ip`.

I filtri opzionali seguono dopo `|`:

| Filter | Example | Effect |
| --- | --- | --- |
| `fixed:N` | `{{dev.123.temperature \| fixed:1}}` | Formattazione decimale con N cifre |
| `format:pattern` | `{{system.time \| format:EEEE HH:mm}}` | Pattern data/ora (`YYYY MM DD HH mm ss EEEE`; testo `[literal]` tra parentesi quadre) |
| `upper` / `lower` / `trim` | `{{system.wifi.station_ip \| upper}}` | Trasformazioni di testo |

Un placeholder che non può essere risolto viene renderizzato come `N/A`
anziché rompere l'intero widget, quindi un sensore temporaneamente assente non
svuota mai il tuo schermo — mostra `N/A` al suo posto.

I dispositivi referenziati dai placeholder diventano vere dipendenze del
registro per il display — il registro ti avviserà prima di cancellare un
sensore che il display sta ancora mostrando.
