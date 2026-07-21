---
title: Pompa di dosaggio
description: Cos'è una dosing pump, perché gli acquariofili la automatizzano e come il dispositivo dosing_pump di Gekko pianifica, calibra e traccia ogni millilitro.
sidebar:
  order: 9
---

## Cos'è una dosing pump?

Una dosing pump è una pompa liquida lenta e precisa. Il tipo più comune è
**peristaltico**: un piccolo motore schiaccia un tubicino morbido in silicone
con dei rulli, spingendo pochi millilitri al secondo. Poiché il liquido tocca
solo il tubo, la pompa non può contaminarlo — e poiché la portata è costante,
*il tempo di funzionamento si traduce direttamente in millilitri*.

Gli acquariofili (e i giardinieri) la usano ovunque un liquido debba essere
aggiunto **poco e spesso**:

- **Reef tank** — calcio, alcalinità (KH), magnesio, elementi traccia. I
  coralli li consumano continuamente; micro-dosi giornaliere mantengono la
  chimica dell'acqua molto più stabile di una grossa correzione settimanale.
- **Planted tank** — fertilizzante liquido quotidiano invece di "quando me lo
  ricordo".
- **Stagni/serre** — tampone pH, nutrienti.

Il dosaggio manuale significa cilindri graduati, calendario e giorni
inevitabilmente saltati. Una dosing pump automatizzata fa lo stesso lavoro
ogni giorno alla stessa ora — ed è proprio questa costanza il punto.

## I pezzi e come Gekko li collega

![Dosing pump setup: container with float sensor, peristaltic pump driven through a relay, ESP32, aquarium](../../../../../assets/diagrams/dosing-setup.svg)

Ti servono quattro pezzi economici, e ognuno mappa a un dispositivo Gekko:

| Hardware | Gekko device | Role |
| --- | --- | --- |
| Pompa peristaltica + scheda relè/MOSFET (filo arancione sopra) | `gpio_switch` (o `port_expander_switch`) | L'uscita che il dispositivo pompa accende e spegne |
| La dosing pump stessa (logica) | `dosing_pump` | Possiede schedule, calibrazione, contenitore e storico |
| Bottiglia/canister con la soluzione | — (tracciata dalla config `dosing_pump`) | Da cui stai dosando |
| Galleggiante opzionale nella bottiglia (filo verde sopra) | `binary_sensor` | Dice a Gekko che la bottiglia è vuota indipendentemente dal contatore |

Più pompe? Crea una catena per ogni liquido — un tipico supporto reef ospita
due o tre pompe (per esempio calcio, alcalinità, magnesio) affiancate, ognuna
con la propria bottiglia e il proprio schedule.

## Configurazione

1. Crea uno **switch GPIO** sul pin che guida il relè della pompa (vedi
   [il tuo primo dispositivo](/gekko/it/getting-started/first-device/) — è lo
   stesso flusso).
2. Crea eventualmente un **binary sensor** per il galleggiante.
3. Crea il dispositivo **dosing pump**: scegli lo switch come *pump switch*,
   il sensore come *low-level sensor* (invertibile per link), imposta la
   capacità del contenitore e la soglia di warning, e aggiungi slot dose allo
   schedule.

![Dosing pump settings in the portal](../../../../../assets/screenshots/device-dosing-pump.png)

## Calibra prima di fidarti

Gekko converte millilitri in secondi di funzionamento tramite un solo numero —
la portata della pompa (`ml/s`). La lunghezza del tubo, il diametro e l'altezza
di mandata la cambiano tutti, quindi misurala una volta con il tuo setup reale:

![Calibration: run a dose, measure the real volume, enter it](../../../../../assets/diagrams/dosing-calibration.svg)

Le esecuzioni di calibrazione sono escluse da statistiche e storico, ma il
liquido erogato **viene** sottratto dal contenitore — è davvero uscito dalla
bottiglia. Se conosci già la portata, una modalità diretta ti consente di
inserirla manualmente.

## Scheduling: come partono davvero le dosi

Lo schedule contiene slot dose (ora del giorno + quantità) e un pattern di
giorni — ogni N giorni, oppure giorni settimana specifici. Il dispositivo lo
valuta rispetto al proprio clock, quindi
[dagli una sorgente temporale affidabile](/gekko/it/reference/devices/schedule/#tempo-e-clock).
Il totale giornaliero viene diviso in più piccole dosi di proposito — ancora
stabilità.

![Dose timeline: on-time dose, dose within the 5-minute grace window, missed dose skipped](../../../../../assets/diagrams/dosing-timeline.svg)

Due policy sono importanti:

- **Grace window.** Uno slot può partire fino a 5 minuti in ritardo — per
  esempio se una dose manuale o una calibrazione stava occupando la pompa nel
  minuto previsto.
- **Drop, don't dose late.** Uno slot mancato oltre la grace window viene
  *saltato*, mai rimandato. Dopo un reboot, un clock che si sincronizza a metà
  giornata o una lunga calibrazione, **non** otterrai una raffica di dosi
  recuperate — per la chimica dell'acqua, una raffica tardiva è peggio di una
  micro-dose persa.

Sempre per slot: **skip next** sopprime esattamente una occorrenza futura
(giorno di cambio acqua, vacanza), e il toggle **auto** blocca l'intero
scheduler mentre le dosi manuali continuano a funzionare.

Una dose, una volta iniziata, gira interamente sul dispositivo — il portale
manda solo il comando. Puoi chiudere il browser a metà dose; il firmware
timerizza l'esecuzione, spegne la pompa e registra la quantità erogata.
Può esserci una sola esecuzione attiva alla volta (manuale, schedulata o
calibrazione — nessuna preempie l'altra), e qualunque cosa tolga il
dispositivo dal servizio ferma forzatamente il motore.

## Tracciamento del contenitore

Dì a Gekko la capacità della bottiglia e terrà il conto di ogni millilitro:

![Container tracking: counter decreases, low-level alert, empty blocks auto dosing](../../../../../assets/diagrams/dosing-container.svg)

- Sotto la **warning threshold** il portale mostra un alert (campanella +
  toast) — è ora di preparare un nuovo batch.
- **Empty** — contatore a zero, o galleggiante scattato — genera un alert
  critico, e con **block auto dosing when empty** attivo, le dosi programmate
  si fermano invece di far girare la pompa a secco.
- **`daysLeft`** proietta per quanto tempo il volume residuo dura alla media
  di consumo giornaliero dello schedule.
- Dopo il refill, registralo con il comando **Set volume**.

## Journal dosi

Ogni dose programmata e manuale viene aggiunta a un journal sul dispositivo —
90+ giorni di storico a un normale ritmo di dosaggio, salvati su una partizione
flash dedicata così sopravvivono a firmware e portal update. La pagina del
dispositivo lo traccia; `GET /api/dosejournal?deviceId=<id>&periodDays=<n>` lo
serve grezzo. Il journal è un ring a dimensione fissa per pompa: i record
vecchi ruotano via da soli, e lo storico di una pompa non può mai
schiacciare quello di un'altra.

## Comandi (REST)

| Command | Payload | Effect |
| --- | --- | --- |
| `startDose` | `amountMl`, `logging` | Dose manuale (`logging:false` = esecuzione di calibrazione) |
| `stopDose` | — | Ferma subito, registra la quantità reale |
| `setVolume` | `volumeMl` | Refill/correzione del contenitore |
| `skipNext` | `doseIndex`, `skip` | Salta una futura occorrenza di uno slot |
| `setMode` | `auto` / `manual` | Abilita/disabilita lo scheduler |

Modello di esecuzione completo e internals del journal:
[`docs/dosing-pump.md`](https://github.com/yoreek/gekko/blob/master/docs/dosing-pump.md).
