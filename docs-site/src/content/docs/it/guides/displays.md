---
title: Display e designer di layout
description: Controlla display OLED SSD1306 e TFT ST7735 con il designer visuale di layout di Gekko e i placeholder di metriche live.
sidebar:
  order: 3
---

Gekko controlla display OLED I2C **SSD1306** e display TFT SPI **ST7735**, e
il portale include un **designer visuale di layout** — componi ciò che lo
schermo mostra da pagine e widget, live nel browser, con un'anteprima.

## Configurare un display

1. Crea prima il dispositivo bus: un
   [**bus I2C**](/gekko/it/reference/devices/i2c-bus/) (pin SDA/SCL) per il
   SSD1306, oppure un [**bus SPI**](/gekko/it/reference/devices/spi-bus/) per lo
   ST7735.
2. Crea il dispositivo display e seleziona quel bus come dipendenza (più
   l'indirizzo I2C o i pin di controllo del TFT).
3. Apri il dispositivo e fai clic su **Design** per entrare nel designer di
   layout.

![Display layout designer](../../../../assets/screenshots/portal-display-designer.png)

## Pagine e widget

Un layout è un insieme di **pagine**; ogni pagina contiene **widget**
posizionati (testo e altro). Il designer mostra un'anteprima live renderizzata
con gli stessi font e metriche usati dal firmware, quindi ciò che vedi è ciò
che il pannello disegna. I layout vengono salvati sul dispositivo e inclusi
nei [bundle di backup](/gekko/it/guides/backup-restore/).

## Valori live: placeholder di metrica

I widget di testo possono mescolare testo statico con **placeholder** risolti
al momento del render. Costruire una schermata di stato è solo scrivere poche
righe di testo template:

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

Un placeholder che non può essere risolto viene renderizzato come testo vuoto
anziché rompere l'intero widget, quindi un sensore temporaneamente assente non
svuota mai il tuo schermo.

I dispositivi referenziati dai placeholder diventano vere dipendenze del
registro per il display — il registro ti avviserà prima di cancellare un
sensore che il display sta ancora mostrando.
