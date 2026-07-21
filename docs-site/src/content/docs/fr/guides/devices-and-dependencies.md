---
title: Périphériques et dépendances
description: Comment fonctionnent le registre de périphériques typé de Gekko et son graphe de dépendances.
sidebar:
  order: 1
---

L'idée centrale de Gekko est un **registre de périphériques** : une liste
persistée d'instances de périphériques, chacune créée à partir d'un des
[types de périphériques](/gekko/reference/devices/) intégrés, avec sa propre
configuration et son état d'exécution en direct.

## Les périphériques sont composés, pas configurés isolément

Le matériel réel est stratifié — un capteur se trouve sur un bus, un
interrupteur derrière un expanseur de ports, une automatisation pilote un
interrupteur. Gekko modélise cela directement : un périphérique
**déclare des dépendances** sur d'autres périphériques, par rôle. Exemples :

| Ce périphérique… | …dépend de |
| --- | --- |
| Sonde de température DS18B20 | un périphérique bus 1-Wire (qui possède le GPIO) |
| Affichage OLED SSD1306 | un périphérique bus I2C |
| Interrupteur sur PCF8574 | le périphérique expanseur de ports |
| Thermostat | un capteur de température **et** un interrupteur |
| Interrupteur automatique | un vrai interrupteur, plus jusqu'à 6 périphériques de condition |
| Sortie analogique programmée | un canal de sortie analogique |

Le registre valide le graphe lorsque vous créez ou modifiez un périphérique —
vous ne pouvez pas connecter un affichage à un périphérique qui n'est pas un
bus I2C, et vous ne pouvez pas supprimer un bus tant qu'un capteur en dépend
encore. Les dépendances se choisissent dans les dialogues de périphérique du
portail, à partir de listes déjà filtrées vers les périphériques compatibles.

## Des rôles, pas des paires codées en dur

Les dépendances sont associées par **rôle** (`switch`, `temperature_sensor`,
`i2c_bus`, `condition`, …), et un type de périphérique peut fournir plusieurs
rôles. Un interrupteur GPIO est à la fois un `switch` et une `condition`, donc
un interrupteur automatique peut l'utiliser soit comme sortie pilotée, soit
comme entrée de condition. Un interrupteur automatique fournit lui aussi
`switch` et `condition`, ce qui permet d'enchaîner les automatisations.

## Config vs état d'exécution

Chaque périphérique sépare :

- **Config** — réglages persistés (nom, broches, règles, dépendances). Stockés
  sur l'appareil en forme binaire versionnée et migrés automatiquement lors
  des mises à jour du firmware. C'est ce que contiennent les
  [lots de sauvegarde](/gekko/guides/backup-restore/).
- **Runtime** — état en direct (on/off, température, statut comme `ready` ou
  `dependency_blocked`). Jamais persisté dans la config ; diffusé au portail en
  temps réel via WebSocket.

Quelques types conservent en plus un petit **état retenu** entre les
redémarrages — par exemple le dernier état de sortie d'un interrupteur
(lorsque « restore previous state » est activé) ou le compte à rebours en pause
d'un interrupteur automatique — sans réécrire leur config.

## Cycle de vie

Les périphériques peuvent être **activés/désactivés** sans être supprimés, et
chaque instance rapporte un statut que le portail affiche : un capteur dont le
bus manque affiche `dependency_blocked`, un périphérique en défaut affiche son
erreur, et le journal des **Device events** enregistre les transitions.
