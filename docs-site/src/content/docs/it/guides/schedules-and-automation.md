---
title: Programmi e automazione
description: Programmi orari e auto switch guidati da condizioni in Gekko.
sidebar:
  order: 2
---

Due tipi di dispositivo lavorano insieme per automatizzare le commutazioni: un
**programma** contiene regole orarie e dice se è attivo adesso, e un **auto
switch** pilota uno switch reale a partire dal AND logico delle condizioni
collegate.

## Programma

Un [dispositivo schedule](/gekko/it/reference/devices/schedule/) contiene fino a
4 regole. Ogni regola ha:

- una **weekday mask** — quali giorni si applica;
- una **time window** — minuto di inizio e fine della giornata (precisione al
  minuto, niente secondi);
- una **mode**:
  - **Always on** — attivo per tutta la finestra;
  - **Interval** — divide la finestra in slice uguali e resta attivo per i
    primi N minuti di ogni slice (per circolazione periodica, nebulizzazione,
    ecc.).

Il programma è attivo quando **qualsiasi** regola abilitata corrisponde.
L'editor regole del portale mostra un'anteprima client-side on/off calcolata
dalle regole rispetto all'orologio del browser — etichettata come stima,
perché il dispositivo valuta le regole rispetto al proprio clock e timezone.

:::note[Give the device a reliable clock]
Gli schedule rifiutano di agire finché il clock del dispositivo non è
plausibile. Usa NTP (imposta il timezone nella pagina **Time**) oppure aggiungi
un dispositivo RTC DS3231 così gli schedule sopravvivono a outage internet e
reboot.
:::

## Auto switch

Un auto switch avvolge uno switch reale (GPIO o espansore di porte) e lo
pilota a partire da fino a **6 condition dependency** — schedule, altri
switch, o altri auto switch — ciascuna opzionalmente **invertita**. Tutte le
condizioni sono in AND: l'output è on solo mentre ogni condizione è soddisfatta.
Senza condizioni, un auto switch in modalità Auto resta spento.

Le sue modalità sono:

- **Off / On** — override manuale; le condizioni vengono ignorate. Il toggle
  dalla dashboard imposta esattamente questo.
- **Auto** — segui le condizioni.
- **Paused** — temporaneamente off per una durata configurata, poi ritorno
  automatico ad **Auto**. La pausa è disponibile solo dalla modalità Auto. Un
  reboot a metà pausa riprende la pausa con il tempo rimanente corretto.

Quando entra in Auto (o Paused), lo switch target viene sempre forzato off
prima, poi consegnato alle condizioni — quindi un vecchio "On" manuale non
rimane mai appeso in silenzio.

Poiché un auto switch agisce lui stesso come switch e come condizione, puoi
collegare le automazioni in catena: un auto switch "feeding mode" può bloccare
più altri auto switch tramite gli slot di condizione invertiti.

## Esempio: luce acquario con pulsante pausa

1. Crea un **Programma** "Light hours", regola: ogni giorno, 09:00–21:00,
   sempre attivo.
2. Crea un **GPIO Switch** "Light relay" sul pin che pilota la tua luce.
3. Crea un **Auto Switch** "Light": target = "Light relay", condition =
   "Light hours", mode = Auto.
4. Fissa "Light" alla dashboard. Ora segue il programma; toccalo per un
   override manuale, usa **pause** durante la manutenzione e torna ad Auto
   quando hai finito.

## Dispositivi correlati

- **[Thermostat](/gekko/it/reference/devices/thermostat/)** — controllo
  temperatura a isteresi che pilota uno switch.
- **[Dosing pump](/gekko/it/reference/devices/dosing-pump/)** — dosaggio
  pianificato con calibrazione, contabilità contenitore e registro dosi.
- **[Scheduled analog output](/gekko/it/reference/devices/analog-outputs/)** —
  curva giornaliera di luminosità/livello per output PWM, componibile in
  fixture multicanale.
