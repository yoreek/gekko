---
title: Striscia di pixel (WS2812B)
description: Strisce RGB indirizzabili WS2812B in Gekko — un backend hardware più effetti a colore fisso e lampeggio di allarme, entrambi controllabili in tempo reale e da Home Assistant.
sidebar:
  order: 13
---

## I mattoncini di base

Una striscia indirizzabile è più di un'uscita on/off o dimmerabile — è un
array di colori. Gekko la modella con un backend hardware e dispositivi di
effetto che la controllano, lo stesso pattern decoratore delle
[uscite analogiche](/gekko/it/reference/devices/analog-outputs/):

| Tipo | Cosa fa |
| --- | --- |
| `pixel_strip` | Un pin dati WS2812B — possiede il buffer dei pixel e lo scrive sull'hardware |
| `pixel_effect_solid` | Riempie la striscia di destinazione con un colore statico |
| `pixel_effect_alert` | Fa lampeggiare la striscia di destinazione mentre le sue condizioni sono soddisfatte |

Ogni effetto prende esattamente una dipendenza `pixel_strip` e la mantiene
in **esclusiva** — non è possibile collegare due effetti alla stessa
striscia contemporaneamente, quindi non se la contendono mai. Gli effetti
non si concatenano ancora tra loro (a differenza di fade/scheduled sulle
uscite analogiche); ogni striscia esegue un solo effetto alla volta.

## `pixel_strip`

Il dispositivo hardware. Configurazione:

- **Pin** — il GPIO collegato alla linea dati della striscia.
- **Numero di pixel** — quanti LED ha la striscia (fino a 300).
- **Luminosità di avvio** — la luminosità applicata all'avvio quando non
  c'è uno stato conservato da ripristinare.
- **Ripristina stato precedente** — avviarsi con l'ultima luminosità
  live invece di partire sempre dal valore di avvio configurato.

Luminosità e on/off sono **stato live**, non fanno parte della
configurazione salvata — trascinare lo slider della dashboard o spegnere il
dispositivo non segna mai la configurazione come modificata né chiede un
salvataggio. Spegnerla mostra sempre nero sull'hardware; riaccenderla
ripristina l'ultima luminosità impostata, quindi non devi mai reinserire un
valore.

## `pixel_effect_solid`

Riempie la sua striscia di destinazione con un colore e lo mantiene — il
modo più semplice per illuminare una striscia con una sola tonalità (un
canale di luce lunare, una luce d'accento, un bianco statico da reef).

- Il selettore di **colore** imposta il colore live direttamente dal
  widget; il selettore di colore nel form di configurazione imposta solo
  il **colore di avvio** applicato al boot.
- **Ripristina stato precedente** funziona esattamente come per
  `pixel_strip`: ripristinare l'ultimo colore live, oppure partire sempre
  dal colore di avvio.
- Lo stesso gate esplicito on/off di `pixel_strip` — spento mostra sempre
  nero sull'hardware indipendentemente dal colore configurato, e
  indipendentemente da quale colore sia attualmente memorizzato.

## `pixel_effect_alert`

Fa lampeggiare la sua striscia di destinazione tra un **colore** configurato
e il nero a un **intervallo di lampeggio** configurato, finché un elenco
limitato di massimo 4 dispositivi con ruolo `Condition` (uno schedule, uno
switch, un auto switch, …) sono tutti soddisfatti — lo stesso meccanismo di
condizioni (AND) usato da
[`auto_switch`](/gekko/it/guides/schedules-and-automation/). Un elenco di
condizioni vuoto non è mai soddisfatto, quindi un allarme mal configurato
non può lampeggiare per errore. A differenza di
`pixel_strip`/`pixel_effect_solid`, qui colore e intervallo di lampeggio
sono normale configurazione persistita — colore e cadenza di un allarme
descrivono cosa *significa* l'allarme, non un valore da regolare live.

Uso tipico: collegare un galleggiante di troppopieno o la condizione
derivata di un `binary_sensor` di perdita a una striscia di allarme rossa
vicino alla vasca.

## Runtime e controllo

`pixel_strip` riporta la sua luminosità live e il numero di pixel;
`pixel_effect_alert` riporta se le sue condizioni sono attualmente
soddisfatte. Uno slider o selettore di colore della dashboard controlla
direttamente luminosità/colore.

Sulle [build MQTT](/gekko/it/guides/mqtt-home-assistant/), tutti e tre i tipi
sono rilevabili in Home Assistant: `pixel_strip` e `pixel_effect_solid` si
pubblicano ciascuno come un `light` (solo luminosità e solo RGB,
rispettivamente), e `pixel_effect_alert` si pubblica come un
`binary_sensor`. Dettagli interni:
[`docs/pixel-strip.md`](https://github.com/yoreek/gekko/blob/master/docs/pixel-strip.md).
