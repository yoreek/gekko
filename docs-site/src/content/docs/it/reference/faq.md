---
title: FAQ e troubleshooting
description: Problemi comuni di flashing, provisioning e uso di un controller Gekko — e le relative soluzioni.
sidebar:
  order: 3
---

## Flashing

### L'installatore web dice che il mio browser non è supportato

Web Serial esiste solo nei browser Chromium — usa **Chrome, Edge o Opera su
desktop**. Firefox, Safari e tutti i browser mobile non possono flashare. In
alternativa usa gli
[script esptool](/gekko/it/getting-started/flashing/), che funzionano ovunque.

### L'installatore non mostra la porta seriale della mia scheda

- Usa un cavo USB **dati** — molti cavi inclusi sono solo per ricarica.
- Installa il driver CP210x o CH340 richiesto dal chip USB-seriale della tua
  scheda.
- Su Linux, aggiungiti al gruppo seriale (`sudo usermod -a -G dialout
  $USER`, poi rifai login) e nota che alcune combinazioni Linux + Chrome +
  chip USB sono note per essere flaky via Web Serial — il percorso esptool è
  il fallback affidabile.
- Chiudi tutto il resto che sta tenendo la porta (serial monitor, IDE).

## Primo avvio e WiFi

### L'access point di setup `gekko-…` non appare mai

- Lascia alla scheda circa 10 secondi dopo l'accensione.
- Se il dispositivo è già stato flashato e ha ancora vecchie credenziali,
  passa direttamente alla modalità station — controlla la lista client del tuo
  router per trovare il suo IP.
- Riflashalo con l'opzione erase (opzione "erase device" dell'installatore
  web, oppure prima `esptool erase_flash` con esptool) per tornare a un primo
  avvio pulito.

### Sono connesso all'AP di setup ma non si apre alcun portale

Non tutti i sistemi aprono automaticamente il captive portal. Apri tu stesso
`http://192.168.4.1/` in un browser.

### Ho salvato credenziali WiFi sbagliate

Non si perde nulla: i retry di connessione dipendono da timeout e l'AP di
setup resta disponibile in parallelo. Riconnettiti all'AP `gekko-…` e correggi
le impostazioni nella pagina WiFi.

## Portale e dispositivi

### Il portale si carica ma un dispositivo mostra `dependency_blocked`

Una delle sue dipendenze è disabilitata, rimossa o in fault — per esempio un
DS18B20 il cui dispositivo bus 1-Wire è disabilitato. Sistema prima il
dispositivo parent; il child si riprende da solo.

### Il mio DS18B20 non compare nella scansione del bus

Controlla la pull-up da circa 4,7 kΩ tra data e 3,3 V, e il cablaggio. Una
sonda sana si scansiona con family code `28` e un indirizzo a 16 caratteri,
senza flag CRC.

### I programmi non accendono mai nulla

Gli schedule richiedono un clock plausibile. Imposta timezone e NTP nella
pagina **Time**, oppure aggiungi una RTC DS3231. Ricorda anche che un auto
switch deve stare in modalità **Auto** — un override manuale Off/On ignora le
condizioni, e un auto switch senza condizioni resta spento per design.

### Dove sono finite le pagine OTA / MQTT?

Quelle pagine appaiono solo sui firmware compilati con la funzionalità
corrispondente — vedi
[Aggiornamenti OTA](/gekko/it/guides/ota-updates/) e
[MQTT e Home Assistant](/gekko/it/guides/mqtt-home-assistant/).

## Ripristino

### Factory reset

Riflashalo con un erase completo (opzione erase dell'installatore web, oppure
`esptool erase_flash` + reflash). Questo cancella credenziali WiFi e tutto il
registro dispositivi — esporta prima un
[backup](/gekko/it/guides/backup-restore/) se vuoi ripristinare il setup dopo.

### Che versione firmware sto eseguendo?

`GET /api/system/version`, la pagina System nel portale, o la riga
`Gekko booting version=…` nel log di boot seriale.
