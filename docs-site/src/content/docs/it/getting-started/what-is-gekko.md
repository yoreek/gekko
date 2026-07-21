---
title: Cos'è Gekko?
description: Un'introduzione a Gekko — un controllore ESP32 modulare con portale web integrato.
sidebar:
  order: 1
---

Gekko è un firmware per ESP32 più un portale web servito direttamente dalla
flash del dispositivo. Insieme ti permettono di costruire il tuo controller —
acquario, terrario, serra o automazione domestica generale — partendo da un
catalogo di tipi di dispositivi, collegati e configurati interamente tramite
l'interfaccia.

**Una sola immagine firmware, nessuna ricompilazione per progetto.** Ogni tipo
di dispositivo supportato è già integrato. Aggiungere un relè, un sensore di
temperatura, un display o una pompa di dosaggio è un'azione nel portale web
sul dispositivo in esecuzione, mai una ricompilazione.

## Cosa puoi costruire con

- **Switch e output** — relè GPIO, switch dietro espansori di porte I2C
  PCF8574/PCF8575, output PWM/analogici con transizioni fade fluide, curve
  giornaliere di luminosità e grouping multicanale.
- **Sensori** — sensori di temperatura DS18B20 (1-Wire) e termistori NTC,
  temperatura + umidità HTU21, e ingressi digitali binari (contatti porta,
  galleggianti, sensori perdite).
- **Automazione** — programmi giornalieri a precisione al minuto, auto switch
  guidati da condizioni con override manuale e pausa, termostati a isteresi e
  pompe di dosaggio con calibrazione e registro dosi.
- **Display** — schermi OLED SSD1306 e TFT ST7735 con un designer visuale di
  pagine/widget integrato nel portale, inclusi placeholder di metriche live
  come `{{dev.123.temperature}}`.
- **Infrastruttura** — bus I2C/SPI/1-Wire, orologio in tempo reale DS3231 e
  dashboard composta da pannelli.

I dispositivi dichiarano **dipendenze** l'uno dall'altro — uno switch su un
espansore di porte, un sensore su un bus I2C, una pompa abilitata da un
programma — e il registro valida, impone e persiste quel grafo. Vedi
[Dispositivi e dipendenze](/gekko/it/guides/devices-and-dependencies/) per il
concetto.

## Cosa rende Gekko diverso

**Nessuna immagine firmware per singola configurazione.** Molti firmware per
controller trasformano la configurazione in una build dedicata — aggiungere un
sensore significa modificare un file di config, ricompilare e riflashare.
Gekko distribuisce una sola immagine con tutti i tipi supportati già inclusi;
cambiare configurazione è sempre un'azione del portale sul dispositivo in
esecuzione, mai una ricompilazione.

**Struttura invece di template di pin.** Configurare a runtime di solito
significa una lista piatta di assegnazioni GPIO e regole console. Gekko invece
modella l'hardware come è davvero cablato: un registro tipizzato di dispositivi
con dipendenze dichiarate, ognuno con la propria config versionata che migra
automaticamente durante gli upgrade firmware.

**Tutto è osservabile e scriptabile.** Lo stato live passa via WebSocket, ogni
dispositivo parla la stessa API REST, gli eventi importanti finiscono in un
registro, i display hanno un designer visuale e la dashboard è composta da
pannelli — non da una singola schermata console.

**Home Assistant con un interruttore.** Sulle build con MQTT, pubblicare un
dispositivo su Home Assistant è un solo switch nella sua pagina — compare come
entità nativa (switch, sensor, climate) controllabile da HA, mentre tutto
continua a girare localmente. Vedi
[MQTT & Home Assistant](/gekko/it/guides/mqtt-home-assistant/).

Il compromesso onesto: il catalogo dei tipi di dispositivi è fisso a compile
time, quindi è volutamente una base più piccola e strutturata, non un catalogo
di ogni sensore esistente.

## Tutto gira sul dispositivo

- **Local-first** — il portale è servito dall'ESP32 via WiFi; niente cloud,
  niente account, niente app store.
- **Integrazioni opzionali** — la discovery MQTT + Home Assistant e gli
  aggiornamenti OTA esistono, ma sono spenti di default.
- **Provisioning WiFi** — un access point di setup (o BLE, quando abilitato)
  porta il dispositivo in rete senza credenziali hardcoded.
- **Backup e ripristino** — tutta la configurazione del dispositivo si esporta
  in un unico bundle modificabile a mano.
- **Sette lingue** — il portale rileva automaticamente la lingua del browser e
  viene fornito in inglese, ucraino, russo, tedesco, spagnolo, francese e
  italiano.

## Prossimi passi

1. [Controlla i requisiti hardware](/gekko/it/getting-started/hardware/)
2. [Flash del firmware](/gekko/it/getting-started/flashing/) — dal browser o con
   esptool/PlatformIO
3. [Collega il dispositivo al WiFi](/gekko/it/getting-started/first-boot-wifi/)
4. [Aggiungi il tuo primo dispositivo](/gekko/it/getting-started/first-device/)
