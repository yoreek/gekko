---
title: Backup e ripristino
description: Esporta tutta la configurazione di un dispositivo Gekko in un unico bundle modificabile a mano e ripristinala ovunque.
sidebar:
  order: 5
---

L'intera configurazione del tuo dispositivo — ogni dispositivo, la sua
configurazione, il grafo delle dipendenze, i layout dei display e la
dashboard — si esporta come **un solo file** che puoi scaricare, conservare,
modificare a mano e ripristinare sullo stesso controller o su un altro.

## Dal portale

Apri **System → Backup**:

- **Download** salva `device-setup.ndjson`.
- **Restore** carica un bundle, con conferma. Il bundle viene validato prima;
  ogni errore rifiuta l'import **senza toccare la configurazione live**, e un
  ripristino riuscito sostituisce atomically tutti i dispositivi.

## Dalla riga di comando

```sh
# backup
curl -fsS http://<device-ip>/api/device-setup/export -o device-setup.ndjson

# restore
curl -fsS -F "bundle=@device-setup.ndjson" http://<device-ip>/api/device-setup/import
```

## Il bundle è JSON puro — e modificabile a mano

Il file è NDJSON: un oggetto JSON per riga, nella stessa forma accettata dalla
REST API. Puoi cambiare un numero di pin in un editor di testo, o persino
scrivere un bundle minimale da zero:

```json
{"kind":"transfer_envelope","transferSchemaVersion":3}
{"kind":"device","record":{"id":4,"typeName":"gpio_switch"},"config":{"name":"Pump","enabled":true,"gpioPin":26}}
```

Solo `record.id` e `record.typeName` sono obbligatori per dispositivo — tutto
il resto riceve i default. Gli id di dipendenza fanno riferimento ad altri
`record.id` nello stesso bundle, e i bundle di firmware più vecchi si importano
pulitamente finché i campi continuano a essere parsabili. Dettagli completi del
formato: [`docs/backup-and-restore.md`](https://github.com/yoreek/gekko/blob/master/docs/backup-and-restore.md).

## Cosa è incluso — e cosa no

**Incluso:** il registro dei dispositivi (tutti i tipi, dipendenze, layout
display) e il layout della dashboard.

**Non incluso:** credenziali WiFi, impostazioni MQTT, stato runtime retained e
stato firmware/OTA.

## Backup automatici

Il firmware non ha volutamente uno scheduler integrato — l'endpoint export è
un semplice GET che qualsiasi macchina della tua LAN può richiamare. Esempio
cron giornaliero:

```sh
# /etc/cron.d/gekko-backup — ogni giorno alle 03:00, conserva 30 giorni
0 3 * * * user curl -fsS http://192.168.1.240/api/device-setup/export \
  -o /var/backups/gekko/device-setup-$(date +\%F).ndjson \
  && find /var/backups/gekko -name 'device-setup-*.ndjson' -mtime +30 -delete
```

Oppure un'automazione Home Assistant con `shell_command` + trigger temporale.
Vedi il documento del repository sopra per uno snippet HA pronto all'uso.

:::caution
Il portale non ha autenticazione — chiunque sulla tua rete può leggere il
bundle (e il portale). Le configurazioni non contengono segreti WiFi/MQTT, ma
tieni il controller su una rete affidabile.
:::
