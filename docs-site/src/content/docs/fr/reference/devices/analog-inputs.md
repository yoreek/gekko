---
title: Entrées analogiques et multiplexeurs
description: Lecture de tensions analogiques dans Gekko — l'ADC natif de l'ESP32, le précis ADS1115 et le multiplexeur CD74HC4067 16 canaux, tous derrière un seul type de canal.
sidebar:
  order: 11
---

## Pourquoi des entrées analogiques ?

Beaucoup de capteurs d'aquarium et de serre ne parlent pas de protocole
numérique — ils sortent simplement une **tension** qui varie avec ce qu'ils
mesurent : une thermistance NTC, une sonde pH ou ORP, une sonde TDS/EC, une
photorésistance, une sonde d'humidité de sol, un capteur de pression ou de
niveau d'eau. Pour les lire, quelque chose doit convertir cette tension en un
nombre — c'est un **ADC** (convertisseur analogique-numérique).

Gekko sépare *d'où vient la tension* (le matériel ADC) de *ce que le nombre
signifie* (une température, un pH, un niveau). Cette page traite de la
première moitié — les quatre types de périphériques qui produisent une lecture
de tension brute. Les capteurs qui interprètent cette lecture, comme la
[thermistance NTC](/gekko/fr/reference/devices/ntc-thermistor/), dépendent de
l'un d'eux et ajoutent le calcul.

Chaque périphérique d'entrée analogique rapporte la même chose : une lecture en
**millivolts** (la valeur de référence) plus un code ADC brut pour le
diagnostic, avec un indicateur de validité. Une entrée déconnectée ou hors
service apparaît comme *invalid*, jamais comme un faux zéro.

## Les trois façons d'obtenir une lecture

| Type | Hardware | Channels | Depends on |
| --- | --- | --- | --- |
| `analog_port_input` | L'ADC natif de l'ESP32 | 1 (sa broche) | — |
| `ads1115_hub` | ADC I2C 16 bits ADS1115 | 4 | un [bus I2C](/gekko/fr/reference/devices/i2c-bus/) |
| `cd74hc4067_hub` | Multiplexeur analogique CD74HC4067 | 16 | — (possède les broches GPIO) |

Lequel convient :

- **`analog_port_input`** — l'option sans composants supplémentaires. L'ESP32
  a déjà un ADC ; cela lit directement l'une de ses broches. Correct pour une
  lecture approximative (photorésistance, flotteur grossier) où ±quelques
  pourcents n'ont pas d'importance. L'ADC intégré n'est que 12 bits et
  légèrement non linéaire, et la moitié de ses broches cessent de fonctionner
  dès que le Wi-Fi est actif (voir les avertissements plus bas).
- **`ads1115_hub`** — quand la précision compte. L'ADS1115 est un ADC I2C
  16 bits avec amplificateur de gain programmable, donc un petit signal (sortie
  d'une carte pH, thermistance précise) est mesuré proprement et de manière
  reproductible. Quatre canaux par puce, et jusqu'à quatre puces sur un seul
  bus (adresses `0x48`–`0x4B`).
- **`cd74hc4067_hub`** — quand vous avez besoin de *beaucoup* de canaux bon
  marché. Le CD74HC4067 est un switch analogique 16 voies : il connecte l'une
  des 16 entrées à une seule broche partagée, que vous reliez à n'importe quel
  ADC (l'ADC natif de l'ESP32, par défaut). Seize sondes d'humidité de sol ou
  niveaux de flotteur sur une seule broche ADC — mais elles partagent tout de
  même la précision de l'ADC intégré et sont lues l'une après l'autre.

## Hubs et canaux

L'entrée de port ESP32 est autonome — elle *est* une lecture, donc il suffit de
la créer et d'y raccorder un capteur.

Les deux puces multi-canaux fonctionnent différemment et reprennent le
[pattern de l'expanseur de ports](/gekko/fr/guides/devices-and-dependencies/) :
le périphérique **hub** possède la puce et ses broches, et chaque canal
réellement utilisé est un périphérique **`analog_input_channel`** séparé qui
dépend du hub.

![Un hub ADS1115 avec quatre canaux ; deux périphériques canal en dépendent et sont lus par des capteurs, le hub dépend d'un bus I2C](../../../../../assets/diagrams/analog-input-hub.svg)

Donc une config ADS1115 à deux sondes = trois périphériques : le
`ads1115_hub`, et deux périphériques `analog_input_channel` (canal 0 et canal
1) qui le ciblent. Chaque canal est nommé, activé et sondé indépendamment — et
chaque canal peut être lu par son propre capteur. Il n'y a **qu'un** seul type
de canal pour les deux familles de hubs : un canal ne nomme jamais la puce
concrète, il demande simplement au hub le "channel N", donc un
`analog_input_channel` fonctionne de la même façon que son hub soit un ADS1115
(canaux 0–3) ou un CD74HC4067 (canaux 0–15). Le formulaire de création borne
le numéro de canal à ce que vous avez réellement sélectionné.

Un périphérique canal est volontairement petit — il nomme juste son hub et son
numéro de canal, puis échantillonne et rapporte des millivolts :

![Réglages d'une entrée analogique : sélecteur de hub, numéro de canal, oversampling et tension en direct](../../../../../assets/screenshots/device-analog-input-channel.png)

Deux canaux ne peuvent pas revendiquer le même numéro sur un même hub —
Gekko refuse le second, comme il refuse deux switches d'expanseur sur la même
broche.

## Mise en place d'un ADS1115

1. Créez un **[bus I2C](/gekko/fr/reference/devices/i2c-bus/)** sur vos broches
   SDA/SCL (si vous n'en avez pas déjà un) et utilisez **Scan bus** pour
   confirmer que l'ADS1115 répond — généralement à `0x48`.
2. Créez un **`ads1115_hub`**, sélectionnez ce bus et réglez son adresse et son
   gain.
3. Pour chaque entrée câblée, créez un **`analog_input_channel`**, sélectionnez
   le hub et choisissez le numéro de canal (0–3 = A0–A3 de l'ADS1115).
4. Raccrochez un capteur (ou regardez simplement les millivolts du canal) à
   chaque canal.

![Réglages du hub ADS1115 : bus I2C, adresse avec scan, gain et débit de données](../../../../../assets/screenshots/device-ads1115-hub.png)

Le **gain** fixe la plage d'entrée et donc la résolution. Choisissez la plus
petite plage qui couvre confortablement votre signal — une plage plus petite
répartit les 16 bits sur moins de volts, donc chaque pas est plus fin :

| Gain | Full-scale range | Use when |
| --- | --- | --- |
| `fsr6144` | ±6,144 V | Jamais nécessaire à 3,3 V — clippe la plage de code |
| `fsr4096` | ±4,096 V | Un signal qui peut atteindre le rail complet 3,3 V |
| `fsr2048` | ±2,048 V | **Défaut** — bon pour la plupart des signaux 0–2 V |
| `fsr1024` | ±1,024 V | Petits signaux sous ~1 V |
| `fsr0512` | ±0,512 V | |
| `fsr0256` | ±0,256 V | Signaux très petits |

:::caution[Ne dépassez pas l'alimentation]
L'ADS1115 peut *représenter* jusqu'à ±6,144 V en code, mais vous ne devez
jamais appliquer à un canal plus que la tension d'alimentation de la puce
(VDD, ici 3,3 V). Le réglage du gain ne fait que choisir comment la plage de
code se mappe sur les volts — il ne protège pas l'entrée.
:::

## Mise en place d'un multiplexeur CD74HC4067

Le CD74HC4067 n'a pas besoin de bus. Il possède quatre **broches d'adresse**
(S0–S3) que Gekko pilote pour sélectionner laquelle des 16 entrées est reliée à
la broche **SIG** partagée, que vous câblez vers une broche ADC (une broche ADC
ESP32, par défaut) :

1. Reliez S0–S3 à quatre GPIO, SIG à une broche compatible ADC, et
   éventuellement EN à une GPIO (reliez EN à GND si vous ne le câblez pas).
2. Créez un **`cd74hc4067_hub`**, saisissez les quatre broches de sélection, la
   broche SIG et son atténuation.
3. Créez un **`analog_input_channel`** par entrée, en sélectionnant le hub et le
   canal 0–15.

![Réglages du hub CD74HC4067 : les quatre broches de sélection S0–S3, la broche d'activation, la broche signal et son atténuation](../../../../../assets/screenshots/device-cd74hc4067-hub.png)

Comme les 16 canaux passent tous par une seule broche ADC ESP32, ils partagent
la précision de cet ADC et la restriction de broche Wi-Fi ci-dessous — le
multiplexeur apporte du **nombre de canaux**, pas de la précision. Les lectures
sont séquentielles : Gekko bascule les lignes d'adresse, attend un tick que le
mux se stabilise, puis échantillonne, donc le scan de nombreux canaux est
naturellement cadencé plutôt qu'instantané.

## Avertissements ADC de l'ESP32 (port input & SIG du CD74HC4067)

`analog_port_input` et la broche SIG du CD74HC4067 utilisent l'ADC intégré de
l'ESP32, qui a deux points à connaître :

- **Utilisez des broches ADC1 avec le Wi-Fi.** Les GPIO **32–39** sont ADC1 et
  restent fonctionnels quand le Wi-Fi tourne ; les broches ADC2 ne le sont pas —
  le Wi-Fi prend l'ADC2, donc une lecture là-bas se bloque ou renvoie du bruit.
  Les GPIO **34–39 sont en entrée seule** (pas de pull-up internes), ce qui est
  exactement ce qu'il faut pour une entrée capteur. La broche par défaut est
  **34**.
- **L'atténuation définit la plage d'entrée.** L'ADC brut mesure jusqu'à ~1,1 V ;
  l'atténuation met les tensions plus grandes dans cette fenêtre. Utilisez la
  plus large (`11db`, par défaut) sauf si votre signal est vraiment petit :

  | Attenuation | Usable input range |
  | --- | --- |
  | `0db` | ~0 – 0,95 V |
  | `2_5db` | ~0 – 1,3 V |
  | `6db` | ~0 – 1,75 V |
  | `11db` | ~0 – 3,1 V (**défaut**, pleine plage) |

Pour tout ce où la tension exacte compte, préférez un ADS1115 — l'ADC intégré
est pratique, pas précis.

## Lissage et remontée

Chaque lecture analogique est la moyenne de plusieurs échantillons ADC pris à
la suite, ce qui réduit le bruit avant même que la valeur ne soit rapportée.
La fréquence d'échantillonnage et l'agressivité de la publication des mises à
jour sont configurables par périphérique/canal — la même idée de delta de
rapport que pour les capteurs de température, pour qu'un signal brut
instable ne noie pas le WebSocket ni les graphiques d'historique.

## Configuration

### `analog_port_input`

| Field | Default | Meaning |
| --- | --- | --- |
| `gpioPin` | `34` | La broche ADC à lire (utilisez ADC1 : 32–39, avec Wi-Fi) |
| `attenuation` | `11db` | Plage d'entrée — voir le tableau ci-dessus |
| `adcSamples` | `8` | Échantillons moyennés par lecture (1–32) |
| `pollMs` | `1000` | Fréquence de lecture |
| `reportDeltaMilliVolts` | `10` | Changement minimal avant envoi d'une nouvelle lecture |
| `reportAlways` | off | Envoie chaque lecture, quel que soit l'écart |

### `ads1115_hub`

| Field | Default | Meaning |
| --- | --- | --- |
| `i2cAddress` | `0x48` | Adresse ADS1115 (`0x48`–`0x4B` par la broche ADDR) |
| `gain` | `fsr2048` | Plage full-scale / PGA — voir le tableau de gain |
| `dataRateSps` | `128` | Échantillons par seconde : `8`–`860` ; plus haut = plus rapide mais plus bruité |

### `cd74hc4067_hub`

| Field | Default | Meaning |
| --- | --- | --- |
| `selectPins` | `[16, 17, 18, 19]` | Les quatre GPIO d'adresse S0–S3 |
| `sigPin` | `34` | Broche ADC recevant la sortie SIG partagée (ADC1 avec Wi-Fi) |
| `sigAttenuation` | `11db` | Plage d'entrée de cette broche ADC — voir le tableau d'atténuation |
| `enablePin` | unused | GPIO EN optionnelle ; laissez vide et reliez EN à GND |

### `analog_input_channel`

| Field | Default | Meaning |
| --- | --- | --- |
| `channel` | `0` | Quel canal du hub (0–3 sur ADS1115, 0–15 sur CD74HC4067) |
| `adcSamples` | `4` | Échantillons moyennés par lecture (1–32) |
| `pollMs` | `1000` | Fréquence de lecture |
| `reportDeltaMilliVolts` | `10` | Changement minimal avant envoi d'une nouvelle lecture |
| `reportAlways` | off | Envoie chaque lecture, quel que soit l'écart |

## Où va la lecture

Une entrée analogique seule n'est qu'une tension sur le tableau de bord —
utile pour un contrôle rapide, mais le but est généralement d'alimenter un
capteur :

- une **[thermistance NTC](/gekko/fr/reference/devices/ntc-thermistor/)**
  transforme la lecture en température, qui peut ensuite piloter un
  [thermostat](/gekko/fr/reference/devices/thermostat/) ;
- les millivolts apparaissent dans les
  [espaces réservés d'affichage](/gekko/fr/guides/displays/) pour tout affichage
  compatible ;
- sur les [builds MQTT](/gekko/fr/guides/mqtt-home-assistant/) chaque entrée
  feuille (l'entrée port et chaque canal) est découvrable dans Home Assistant
  comme capteur `voltage`. Les hubs ne le sont pas — ils fournissent des
  canaux, pas une lecture propre.

Internals firmware :
[`docs/analog-input.md`](https://github.com/yoreek/gekko/blob/master/docs/analog-input.md).
