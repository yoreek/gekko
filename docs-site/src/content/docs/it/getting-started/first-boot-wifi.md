---
title: Primo avvio e setup WiFi
description: Collega un controller Gekko appena flashato alla tua rete WiFi tramite il suo access point di setup.
sidebar:
  order: 4
---

Gekko viene fornito **senza credenziali WiFi hardcoded**. Al primo avvio il
dispositivo apre il proprio access point di setup e configuri la rete dal
portale.

## Collegarsi tramite l'access point di setup

1. Accendi la scheda appena flashata. Entro pochi secondi avvia un access
   point WiFi aperto chiamato **`gekko-<suffix>`**, dove il suffisso deriva
   dall'indirizzo MAC della scheda — così due controller vicini non collidono
   mai.
2. Collegati a quell'access point da telefono o laptop. Sulla maggior parte
   dei sistemi appare un prompt captive-portal; se non succede, apri il
   portale direttamente via IP — `http://192.168.4.1/` (l'indirizzo AP ESP32
   predefinito).
3. Apri la pagina **WiFi** nel portale. Il dispositivo scansiona le reti
   vicine e mostra un elenco.
4. Scegli la tua rete, inserisci la password e salva.
5. Il dispositivo si connette alla rete come station. L'access point di setup
   è gestito dalla state machine WiFi — resta disponibile finché la connessione
   station non è stabilita, quindi un errore nella password non ti blocca mai.

Dopo una connessione riuscita, apri il portale all'indirizzo assegnato dal
router al dispositivo (controlla la lista client del router o la riga di log
seriale del dispositivo). Da quel momento il portale è servito sulla tua rete
normale.

## Se la connessione fallisce

Le credenziali salvate per una rete non raggiungibile non **brickano** il
dispositivo: i retry di connessione station sono guidati da timeout, e l'AP di
setup insieme al portale restano disponibili per tutto il tempo — ricollegati
all'AP e correggi le impostazioni.

## Alternativa: provisioning BLE

Le build con provisioning mobile abilitato possono anche ricevere credenziali
WiFi via **Bluetooth LE** usando un'app di provisioning compatibile con
Espressif (Android/iOS). La modalità config BLE parte solo dopo una richiesta
esplicita dal portale o dall'API, gira con un timeout di sessione e non
modifica mai le credenziali salvate a meno che l'app non invii con successo
nuovi dati. Se hai flashato l'immagine predefinita, usa il flusso AP di setup
qui sopra — è sempre disponibile.

## Prossimo passo

Con il dispositivo in rete, fai il
[tour del portale](/gekko/it/getting-started/portal-tour/) oppure vai
direttamente ad [aggiungere il tuo primo dispositivo](/gekko/it/getting-started/first-device/).
