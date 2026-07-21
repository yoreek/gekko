---
title: GPIO-Schalter
description: Referenz fuer den Gekko-Geraetetyp gpio_switch - ein Ein/Aus-Ausgang auf einem ESP32-GPIO-Pin.
sidebar:
  order: 2
---

`gpio_switch` steuert einen GPIO-Pin als Ein/Aus-Ausgang - Relaisboards,
MOSFET-Module, Status-LEDs. Es ist oft das erste Geraet, das du anlegst; der
[erste-Geraet-Rundgang](/gekko/de/getting-started/first-device/) verwendet es.

![GPIO-Schalter-Einstellungen](../../../../../assets/screenshots/device-gpio-switch.png)

## Abhaengigkeiten

Keine - es besitzt seinen GPIO-Pin direkt. (Fuer Ausgaenge hinter einem
PCF8574-/PCF8575-Expander nimm stattdessen `port_expander_switch`; es bietet
die gleichen Schalteroptionen unten.)

## Konfiguration

| Feld | Standard | Bedeutung |
| --- | --- | --- |
| `gpioPin` | `4` | Der Ausgangspin |
| `inverted` | aus | Elektrischen Pegel invertieren - fuer aktiv-low-Relaisboards einschalten |
| `startupState` | aus | Ausgangszustand direkt nach dem Boot (wenn nicht wiederhergestellt wird) |
| `restorePreviousState` | aus | Den letzten Zustand vor dem Neustart wiederherstellen statt `startupState` zu verwenden |
| `safeState` | aus | Zustand, auf den bei Unverfuegbarkeit eines steuernden Geraets zurueckgefallen wird |
| `enabled` | an | Deaktivierte Geraete geben ihren Ausgang frei und melden nichts mehr |

## Laufzeit & Steuerung

Das Geraet meldet seinen Live-Ein/Aus-Zustand; schalte es aus der Geraeteliste,
einem Dashboard-Schalter-Widget, der REST-API oder Home Assistant (als
`switch`-Entity auf [MQTT-Builds](/gekko/de/guides/mqtt-home-assistant/)).

## Bereitgestellt

- **switch** - kann von Thermostat, Auto-Switch oder Dosierpumpe gesteuert werden.
- **condition** - sein Ein/Aus-Zustand kann einen Auto-Switch sperren.
