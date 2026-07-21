---
title: Expanseurs de ports (PCF8574 / PCF8575)
description: Ajouter davantage de sorties d'interrupteur via I2C dans Gekko — les expanseurs de ports PCF8574 et PCF8575 et le port_expander_switch qui pilote l'une de leurs broches.
sidebar:
  order: 2.5
---

## Pourquoi un expanseur de ports ?

L'ESP32 a déjà beaucoup de GPIO, mais un bac chargé peut quand même en
manquer — chaque relais, chaque MOSFET veut une broche, et certaines broches
sont déjà prises (bus I2C, écran SPI, broches ADC en entrée seule). Un
**expanseur de ports** est la solution économique : une puce I2C qui vous donne
**8 ou 16 broches d'E/S supplémentaires sur les deux mêmes fils** que le reste
de vos périphériques I2C partage déjà. Ajoutez un expanseur et vous obtenez un
bloc de sorties relais sans toucher une autre broche ESP32.

Gekko prend en charge les deux plus courants :

| Type | Chip | Extra pins | Addresses |
| --- | --- | --- | --- |
| `pcf8574_expander` | PCF8574 | 8 | `0x20`–`0x27` |
| `pcf8575_expander` | PCF8575 | 16 | `0x20`–`0x27` |

Les deux sont des hubs au rôle `port_expander` sur un
[bus I2C](/gekko/fr/reference/devices/i2c-bus/) ; la seule différence est 8 vs 16
broches. Jusqu'à huit de chaque peuvent partager un bus, leurs adresses étant
définies par les cavaliers de soudure A0/A1/A2.

## Hub et canaux

Comme pour les [hubs ADS1115 / multiplexeur](/gekko/fr/reference/devices/analog-inputs/),
un expanseur est un **hub** : le périphérique expander possède la puce, et
chaque broche de sortie réellement utilisée est un périphérique
**`port_expander_switch`** séparé qui en dépend.

Donc une config PCF8574 à deux relais = trois périphériques : le
`pcf8574_expander`, et deux périphériques `port_expander_switch` (pin 0 et pin
1) qui le ciblent. Chaque switch est nommé, activé et contrôlable
indépendamment — et se comporte exactement comme un
[interrupteur GPIO](/gekko/fr/reference/devices/gpio-switch/), juste sur une
broche d'expanseur au lieu d'une broche ESP32. Deux switches ne peuvent pas
revendiquer la même broche sur un même expanseur ; Gekko refuse le second.

Comme un `port_expander_switch` **fournit le rôle `switch`**, tout ce qui
pilote un switch le pilote aussi — un
[thermostat](/gekko/fr/reference/devices/thermostat/), un `auto_switch`, une
[dosing pump](/gekko/fr/reference/devices/dosing-pump/). Rien dans ces
contrôleurs ne sait ni ne se soucie que la sortie soit derrière un expanseur.

## Mise en route

1. Créez un **[bus I2C](/gekko/fr/reference/devices/i2c-bus/)** (si vous n'en
   avez pas) et utilisez **Scan bus** pour confirmer que l'expanseur répond —
   généralement à `0x20`.
2. Créez un **`pcf8574_expander`** (ou `pcf8575_expander`), sélectionnez ce
   bus et réglez son adresse.
3. Pour chaque sortie, créez un **`port_expander_switch`**, sélectionnez
   l'expanseur et choisissez le numéro de broche (0–7 sur un PCF8574, 0–15 sur
   un PCF8575).

![Réglages PCF8574 : bus I2C, adresse avec scan et option de polarité](../../../../../assets/screenshots/device-pcf8574-expander.png)

Puis le switch lui-même, avec les mêmes options qu'un switch GPIO :

![Réglages du switch sur expanseur : sélecteur d'expanseur, numéro de broche et options du switch](../../../../../assets/screenshots/device-port-expander-switch.png)

## Cartes relais actives bas

La plupart des cartes relais bon marché sont **actives bas** — le relais se
ferme quand la broche est tirée *bas*, pas haut. Il y a deux endroits pour
corriger cela, et il vaut mieux être intentionnel :

- **Sur l'expanseur** — son option `inverted` inverse la polarité électrique de
  *toute* la puce. Utilisez-la quand toute la carte est active bas.
- **Sur le switch** — son option `inverted` inverse une *seule* broche.
  Utilisez-la quand seules certaines broches sont câblées active bas.

Faites-le correctement et "on" dans Gekko signifie que le relais est
réellement alimenté.

## Configuration

### `pcf8574_expander` / `pcf8575_expander`

| Field | Default | Meaning |
| --- | --- | --- |
| `i2cAddress` | `0x20` | Adresse de la puce (`0x20`–`0x27` via les cavaliers A0/A1/A2) |
| `inverted` | off | Inverse le niveau électrique de chaque broche (cartes actives bas) |
| `enabled` | on | Désactiver l'expanseur libère tous les switches qui en dépendent |

### `port_expander_switch`

| Field | Default | Meaning |
| --- | --- | --- |
| `channel` | `0` | Quelle broche de l'expanseur (0–7 sur PCF8574, 0–15 sur PCF8575) |
| `inverted` | off | Inverse le niveau électrique de cette seule broche |
| `startupState` | off | État de sortie juste après le boot (quand la restauration est désactivée) |
| `restorePreviousState` | off | Restaure le dernier état d'avant le redémarrage au lieu de `startupState` |
| `safeState` | off | État de repli quand un périphérique contrôleur devient indisponible |
| `enabled` | on | Les switches désactivés libèrent leur broche et cessent de rapporter |

## Fournit

Un `port_expander_switch` fournit les mêmes rôles qu'un interrupteur GPIO :

- **switch** — peut être piloté par un thermostat, un auto switch ou une dosing pump ;
- **condition** — son état on/off peut bloquer un auto switch.

Sur les [builds MQTT](/gekko/fr/guides/mqtt-home-assistant/), chaque switch est
découvrable dans Home Assistant comme entité `switch`. L'expanseur lui-même
ne l'est pas — il fournit des broches, pas un contrôle propre.
