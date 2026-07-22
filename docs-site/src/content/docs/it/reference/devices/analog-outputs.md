---
title: Uscite analogiche e compositore luci
description: Uscite PWM dimmerabili in Gekko — fade fluidi, curve giornaliere di luminosità e fixture multi-canale come una luce per acquario a cinque canali.
sidebar:
  order: 10
---

## Perché uscite dimmerabili?

Uno switch on/off va bene per un riscaldatore — ma una luce non dovrebbe
passare di colpo da 0 a 100% alle 9 del mattino. Una buona luce per acquario
(o terrario, o serra):

- **sale e scende gradualmente** — alba e tramonto, non un interruttore; i
  pesci si spaventano visibilmente con transizioni brusche, e nemmeno i coralli
  le apprezzano;
- **cambia intensità durante il giorno** — un picco a mezzogiorno, mattine e
  sere più dolci;
- **miscela più canali colore** — i fixture reef hanno spesso stringhe LED
  separate royal blue, blue, white, violet e moonlight, ciascuna con la propria
  curva giornaliera.

Gekko modella tutto questo con quattro tipi di dispositivo che si incastrano
come blocchi. Ogni livello è una percentuale (0–100%) nel portale e nell'API;
la parte hardware è un pin PWM (LEDC) ESP32 che guida l'ingresso di dimming di
un driver LED, un modulo MOSFET o qualsiasi altro carico controllato via PWM.

## I blocchi costitutivi

| Type | What it does |
| --- | --- |
| `analog_output` | Il canale PWM hardware su un pin |
| `fade_analog_output` | Smussa ogni cambiamento in una rampa graduale |
| `scheduled_analog_output` | Guida il proprio target lungo una curva giornaliera |
| `analog_output_composer` | Raggruppa più canali in un solo fixture |

Un'uscita fade o scheduled prende esattamente una dipendenza con ruolo
`analog_output` e *fornisce lo stesso ruolo* a sua volta, quindi si
impilano:

![Decorator chain: scheduled output computes the level, fade smooths it, analog output writes PWM](../../../../../assets/diagrams/analog-chain.svg)

- **Fade** — `maxStep` (percentuale per step) e `stepIntervalMs` definiscono la
  velocità della rampa; il default ≈1% ogni 200 ms trasforma ogni cambiamento,
  incluso uno spostamento manuale dello slider, in una transizione dolce.
- **Scheduled** — fino a 10 punti `(time, level)` al giorno, interpolati tra i
  punti. Modalità: **Off**, **Manual** (livello fisso), **Scheduled** (segui
  la curva). Senza clock valido, l'uscita va a zero invece di tenere un livello
  vecchio.

Il registro impone che ogni uscita abbia **al massimo un controller** — non
puoi accidentalmente cablare due schedule sullo stesso canale.

## Esempio completo: una luce per acquario a cinque canali

L'obiettivo — una giornata che assomiglia a questa:

![Daily curves of five channels: blues ramp first and linger, white peaks midday, violet accents, moonlight at night](../../../../../assets/diagrams/aquarium-light-day.svg)

I blu salgono per primi e scendono per ultimi (i coralli fotosintetizzano
principalmente nel blu), il bianco caldo riempie le ore di mezzogiorno, il
violet aggiunge pop fluorescente e un tenue canale moonlight brilla di notte.
Per costruirla:

1. Crea cinque dispositivi **`analog_output`**, uno per pin driver LED:
   "Royal blue LEDC", "Blue LEDC", "White LEDC", "Violet LEDC", "Moonlight
   LEDC".
2. Avvolgi ognuno in un **`fade_analog_output`** ("Royal blue fade" → target
   "Royal blue LEDC", …) così i cambi di canale non saltano mai.
3. Avvolgi ogni fade in un **`scheduled_analog_output`** ("Royal blue schedule"
   → target "Royal blue fade", …) e disegna la curva giornaliera di quel
   canale.
4. Crea un solo **`analog_output_composer`** "Aquarium light" e aggiungi i
   cinque output schedulati come suoi canali.

![Aquarium light composer in the portal](../../../../../assets/screenshots/device-analog-composer.png)

Il composer ora si comporta come *la* luce:

- **Una sola modalità per tutto il fixture** — passare il composer tra Off /
  Manual / Scheduled spinge quella modalità a tutti i canali e li mantiene
  sincronizzati se uno diverge. Off azzera tutto.
- **Un solo editor** — tutte le curve dei canali su un unico grafico, editate
  in place trascinando i punti (clic destro per inserire/cancellare, snap
  opzionale 15 min/5%, overlay alba/tramonto come riferimento); la modalità
  manual mostra uno slider per canale.
- **Una sola card dashboard** — fissa il composer per un'anteprima compatta
  dello schedule multi-canale.

Il composer serve solo quando più canali devono agire come un unico fixture —
una luce a singolo canale è solo gli step 1–3 con una sola catena. Salta il
layer fade se non ti interessano le rampe.

Per cablaggio, configurazione e verifica, consulta il progetto
[Lampada multicanale](/gekko/it/projects/multichannel-light/).

## Runtime e controllo

Tutti e quattro i tipi riportano il loro livello live (i fade riportano anche
il target e se sono ancora in transizione). Uno slider della dashboard o il
comando `setOutput` guida direttamente un canale; i cambi di modalità passano
per `setMode`. Sulle [build MQTT](/gekko/it/guides/mqtt-home-assistant/) i canali
sono scopribili in Home Assistant. Internals:
[`docs/analog-output.md`](https://github.com/yoreek/gekko/blob/master/docs/analog-output.md).
