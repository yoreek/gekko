---
title: Aggiornamenti OTA
description: Aggiornamenti firmware over-the-air in Gekko — cosa funziona sulle schede da 4 MB e cosa richiede più flash.
sidebar:
  order: 6
---

Gekko ha due percorsi over-the-air, entrambi **spenti nella build
predefinita** — la classica scheda ESP32 da 4 MB semplicemente non ha margine
di flash per uno schema di partizioni OTA accanto a firmware, portale web e
configurazione.

## Build predefinita: update via seriale

Sulla classica layout single-app, gli aggiornamenti sono un
[reflash USB](/gekko/it/getting-started/flashing/). La configurazione del
dispositivo è salvata in NVS e sopravvive al reflash del firmware — e dovresti
comunque tenere un
[backup bundle](/gekko/it/guides/backup-restore/) prima di aggiornare.

## Upload OTA di PlatformIO (sviluppatori)

Per le schede con abbastanza flash e un layout partizioni abilitato per OTA,
l'ambiente PlatformIO `esp32dev_ota` consegna la stessa immagine firmware sulla
rete invece che via seriale:

```sh
pio run -e esp32dev_ota -t upload
```

È volutamente un alias di trasporto upload di `esp32dev` — stessa immagine,
stessi flag di build — così le consegne seriali e OTA restano byte-identical.

## Web OTA (upload dal portale)

Il firmware compilato con l'opzione Web OTA protetta aggiunge una pagina
**OTA** al portale: carica un'immagine firmware dal browser e il dispositivo la
verifica, la finalizza e si riavvia dentro di essa. Upload troppo grandi o
interrotti lasciano intatto il firmware in esecuzione. Nelle build senza la
funzionalità, il portale semplicemente nasconde la pagina OTA.

:::note
Considera Web OTA una funzionalità di sviluppo / avanzata: abilitala solo su
schede con margine di flash, secondo `docs/platformio-environments.md` nel
repository.
:::
