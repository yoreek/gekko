---
title: MQTT & Home Assistant
description: Ein Schalter pro Geraet und es erscheint in Home Assistant - Schalter, Sensoren und Thermostate, steuerbar aus der HA-Oberflaeche.
sidebar:
  order: 4
---

Gekko spricht **Home-Assistant-MQTT-Discovery**: Verbinde es einmal mit
deinem MQTT-Broker, dann veroeffentliche jedes Geraet mit einem einzigen
Schalter - und es erscheint in Home Assistant mit dem richtigen Entity-Typ,
Namen und Icon. Kein YAML, keine manuelle Entity-Konfiguration.

## Was du bekommst

![Gekko-Geraete erscheinen in Home Assistant als Schalter-, Sensor- und Climate-Entities](../../../../assets/diagrams/ha-entities.svg)

Jedes veroeffentlichte Gekko-Geraet wird zu einer nativen HA-Entity, und die
Steuerung laeuft in beide Richtungen in Echtzeit:

| Gekko-Geraet | In Home Assistant | Du kannst ... |
| --- | --- | --- |
| GPIO / Portexpander / Auto-Switch | `switch` | es aus jedem HA-Dashboard schalten und in Automationen nutzen |
| DS18B20, NTC-Thermistor | `sensor` | Historie charten, Automationen auf Temperatur ausloesen |
| HTU21 | zwei `sensor`s (Temperatur + Luftfeuchte) | das gleiche, unabhaengig |
| Binarsensor | `binary_sensor` | Leck-/Tuerenalarme per HA-Benachrichtigung erhalten |
| Thermostat | `climate` | Modus und Sollwert aus der Thermostat-Karte von HA aendern |

So kann deine Aquariumbeleuchtung in HA-Szenen auftauchen, der Lecksensor kann
eine Push-Nachricht senden, und das Thermostat erscheint neben den
Klimasteuerungen deines Hauses - waehrend alles weiter lokal auf dem ESP32
laeuft, selbst wenn HA ausfaellt.

## Einrichtung

1. **Broker verbinden (einmalig).** Auf der **MQTT / Home Assistant**-Seite
   im Portal Host, Port und Zugangsdaten deines Brokers eintragen (TLS wird
   unterstuetzt) und **Enable MQTT** einschalten. Aenderungen verbinden sauber
   neu - kein Reboot. MQTT verbindet sich nur, wenn das Geraet als Station im
   WiFi ist, nie im Setup-AP-Modus.

   ![MQTT-Broker-Einstellungen](../../../../assets/screenshots/portal-mqtt.png)

2. **Sicherstellen, dass HA denselben Broker nutzt** und in der MQTT-Integration
   Discovery aktiviert ist (Standard).

3. **Geraete veroeffentlichen.** Jede unterstuetzte Geraeteseite hat eine
   **Home Assistant**-Karte - **Publish to Home Assistant** einschalten,
   optional einen HA-Namen vergeben und speichern:

   ![Home-Assistant-Karte pro Geraet mit Publish-Schalter](../../../../assets/screenshots/device-ha-card.png)

   Wenige Sekunden spaeter erscheint das Geraet in HA unter **Settings -> Devices
   & services -> MQTT**, gruppiert unter deinem Gekko-Controller. Beim
   Unpublishen verschwindet es genauso sauber.

## Eine Build-Option

MQTT-Unterstuetzung wird bei Bedarf in die Firmware einkompiliert
(`-DWITH_HOME_ASSISTANT` in `platformio.ini`) - Firmware ohne dieses Feature
enthaelt gar keinen MQTT-Code, was auf 4-MB-Boards wichtig ist. Das Portal
zeigt den Unterschied klar:

- das **Available / Not available**-Badge auf der MQTT-Seite zeigt, ob dieser
  *Build* das Feature hat;
- der **Enable MQTT**-Schalter sagt der Firmware, ob sie jetzt wirklich
  verbinden soll.

Auf Builds ohne das Feature zeigt die MQTT-Seite einen Hinweis, und die
per-device HA-Karten werden nicht gerendert.

Die komplette Architektur (Topic-Schema, Adapter, TLS-Zertifikate) steht in
[`docs/mqtt-home-assistant.md`](https://github.com/yoreek/gekko/blob/master/docs/mqtt-home-assistant.md)
im Repository.
