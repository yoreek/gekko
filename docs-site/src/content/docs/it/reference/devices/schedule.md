---
title: Schedule
description: Riferimento per il tipo di dispositivo schedule di Gekko — regole orarie giornaliere e per giorni settimana con precisione al minuto.
sidebar:
  order: 8
---

`schedule` contiene un insieme di regole temporali e risponde a una sola
domanda: *questo schedule è attivo adesso?* Non ha un output proprio —
attaccalo come condizione a un [auto switch](/gekko/it/guides/schedules-and-automation/)
(o alla pianificazione di una dosing pump) per far accadere qualcosa.

![Schedule rule editor](../../../../../assets/screenshots/device-schedule.png)

## Dipendenze

Nessuna. Sono gli altri dispositivi che dipendono dallo schedule, non il
contrario.

## Configurazione

Fino a **4 regole**, in OR tra loro — lo schedule è attivo quando una
qualsiasi regola abilitata corrisponde. Ogni regola:

| Field | Meaning |
| --- | --- |
| Weekdays | Quali giorni della settimana si applica la regola |
| Start / end time | Finestra attiva, in minuti del giorno (precisione al minuto — non ci sono secondi) |
| Mode | **Always on** — attivo per l'intera finestra; **Interval** — divide la finestra in N intervalli uguali, attivo per i primi M minuti di ciascuno |

La modalità interval copre compiti periodici: per es. una finestra 08:00–20:00
con 12 intervalli e durata di 5 minuti fa girare una pompa di circolazione 5
minuti ogni ora.

## Tempo e clock

Le regole vengono valutate rispetto all'orologio del dispositivo e al suo
timezone configurato (con DST gestito automaticamente). Finché il clock non è
plausibile — NTP sincronizzato o RTC DS3231 presente — lo schedule si segnala
come non valido e i dispositivi dipendenti tengono le uscite in stato sicuro.

L'editor del portale mostra un'anteprima on/off e la prossima transizione,
calcolate nel browser dalle stesse regole; è etichettata come stima perché il
clock e il fuso del tuo browser possono differire da quelli del dispositivo.

## Fornisce

- **condition** — per auto switch e automazioni a catena.
- **schedule** — per dispositivi che consumano direttamente gli schedule.
