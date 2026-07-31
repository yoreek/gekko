---
title: Ruban de pixels (WS2812B)
description: Rubans RGB adressables WS2812B dans Gekko — un backend matériel plus des effets de couleur unie et de clignotement d'alerte, tous deux pilotables en direct et depuis Home Assistant.
sidebar:
  order: 13
---

## Les briques de base

Un ruban adressable est plus qu'une sortie on/off ou variable — c'est un
tableau de couleurs. Gekko le modélise avec un backend matériel et des
appareils d'effet qui le ciblent, le même patron décorateur que les
[sorties analogiques](/gekko/fr/reference/devices/analog-outputs/) :

| Type | Ce qu'il fait |
| --- | --- |
| `pixel_strip` | Une broche de données WS2812B — possède le tampon de pixels et l'écrit sur le matériel |
| `pixel_effect_solid` | Remplit le ruban cible d'une couleur statique |
| `pixel_effect_alert` | Fait clignoter le ruban cible tant que ses conditions sont remplies |

Chaque effet prend exactement une dépendance `pixel_strip` et la détient de
manière **exclusive** — impossible de câbler deux effets sur le même ruban à
la fois, ils ne se disputent donc jamais son contrôle. Les effets ne se
chaînent pas encore entre eux (contrairement à fade/scheduled sur les
sorties analogiques) ; chaque ruban n'exécute qu'un seul effet à la fois.

## `pixel_strip`

L'appareil matériel. Configuration :

- **Broche** — le GPIO câblé à la ligne de données du ruban.
- **Nombre de pixels** — combien de LED compte le ruban (jusqu'à 300).
- **Luminosité de démarrage** — la luminosité appliquée au démarrage quand
  aucun état retenu n'est disponible à restaurer.
- **Restaurer l'état précédent** — démarrer avec la dernière luminosité en
  direct au lieu de toujours démarrer avec la valeur de démarrage
  configurée.

Luminosité et on/off sont un **état en direct**, pas une partie de la
configuration enregistrée — faire glisser le curseur du tableau de bord ou
éteindre l'appareil ne marque jamais la configuration comme modifiée et ne
demande jamais de dialogue d'enregistrement. L'éteindre affiche toujours du
noir au niveau matériel ; le rallumer restaure la dernière luminosité
définie, vous n'avez donc jamais à ressaisir une valeur.

## `pixel_effect_solid`

Remplit son ruban cible d'une couleur et la maintient — la façon la plus
simple d'éclairer un ruban d'une seule teinte (un canal clair de lune, une
lumière d'accent, un blanc récifal statique).

- Le sélecteur de **couleur** définit la couleur en direct directement
  depuis le widget ; le sélecteur de couleur du formulaire de configuration
  ne définit que la **couleur de démarrage** appliquée au démarrage.
- **Restaurer l'état précédent** fonctionne exactement comme pour
  `pixel_strip` : restaurer la dernière couleur en direct, ou toujours
  démarrer depuis la couleur de démarrage.
- Le même verrou explicite on/off que `pixel_strip` — éteint affiche
  toujours du noir au niveau matériel quelle que soit la couleur
  configurée, indépendamment de la couleur actuellement stockée.

## `pixel_effect_alert`

Fait clignoter son ruban cible entre une **couleur** configurée et le noir à
un **intervalle de clignotement** configuré, tant qu'une liste bornée
d'au plus 4 appareils de rôle `Condition` (un horaire, un interrupteur, un
auto switch, …) sont tous remplis — le même mécanisme de conditions (ET)
qu'utilise [`auto_switch`](/gekko/fr/guides/schedules-and-automation/). Une
liste de conditions vide n'est jamais remplie, une alerte mal configurée ne
peut donc pas clignoter par accident. Contrairement à
`pixel_strip`/`pixel_effect_solid`, la couleur et l'intervalle de
clignotement sont ici de la configuration persistée classique — la couleur
et la cadence d'une alerte décrivent ce que l'alerte *signifie*, pas une
valeur qu'on ajusterait en direct.

Usage typique : câbler un flotteur de débordement ou la condition dérivée
d'un `binary_sensor` de fuite dans un ruban d'alerte rouge près du bac.

## Exécution & contrôle

`pixel_strip` rapporte sa luminosité en direct et son nombre de pixels ;
`pixel_effect_alert` rapporte si ses conditions sont actuellement remplies.
Un curseur ou sélecteur de couleur du tableau de bord pilote directement la
luminosité/couleur.

Sur les [builds MQTT](/gekko/fr/guides/mqtt-home-assistant/), les trois
types sont détectables dans Home Assistant : `pixel_strip` et
`pixel_effect_solid` se publient chacun comme un `light` (luminosité
seule et RGB seule, respectivement), et `pixel_effect_alert` se publie
comme un `binary_sensor`. Détails internes :
[`docs/pixel-strip.md`](https://github.com/yoreek/gekko/blob/master/docs/pixel-strip.md).
