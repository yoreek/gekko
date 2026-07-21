---
title: Interrupteur GPIO
description: Référence du type de périphérique gpio_switch de Gekko — une sortie on/off sur une broche GPIO ESP32.
sidebar:
  order: 2
---

`gpio_switch` pilote une broche GPIO comme sortie on/off — cartes relais,
modules MOSFET, LED d'état. C'est généralement le premier périphérique que
vous créez ; le [parcours du premier périphérique](/gekko/fr/getting-started/first-device/)
l'utilise.

![Réglages de l'interrupteur GPIO](../../../../../assets/screenshots/device-gpio-switch.png)

## Dépendances

Aucune — il possède directement sa propre broche GPIO. (Pour des sorties
derrière un expanseur PCF8574/PCF8575, utilisez plutôt
`port_expander_switch` ; il propose les mêmes options de switch ci-dessous.)

## Configuration

| Field | Default | Meaning |
| --- | --- | --- |
| `gpioPin` | `4` | La broche de sortie |
| `inverted` | off | Inverse le niveau électrique — à activer pour les cartes relais actives bas |
| `startupState` | off | État de sortie juste après le boot (quand la restauration est désactivée) |
| `restorePreviousState` | off | Restaure le dernier état d'avant le redémarrage au lieu de `startupState` |
| `safeState` | off | État de repli quand un périphérique contrôleur devient indisponible |
| `enabled` | on | Les périphériques désactivés libèrent leur sortie et cessent de rapporter |

## Runtime et contrôle

Le périphérique rapporte son état on/off en direct ; basculez-le depuis la
liste des périphériques, un widget switch du tableau de bord, l'API REST ou
Home Assistant (comme entité `switch` sur les
[builds MQTT](/gekko/fr/guides/mqtt-home-assistant/)).

## Fournit

- **switch** — peut être piloté par un thermostat, un auto switch ou une
  dosing pump.
- **condition** — son état on/off peut bloquer un auto switch.
