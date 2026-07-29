---
title: Capteur de température à thermistance NTC
description: Lecture de température avec une thermistance NTC bon marché dans Gekko — le pont diviseur, les préréglages, les courbes Beta et Steinhart-Hart, et l'étalonnage.
sidebar:
  order: 12
---

## Qu'est-ce qu'une thermistance NTC ?

Une thermistance NTC est une résistance dont la résistance **baisse quand elle
chauffe** (NTC = Negative Temperature Coefficient). C'est le capteur de
température le moins cher qui existe — quelques centimes — et il existe en
forme de pastille de verre, d'époxy ou de sonde étanche. La différence avec un
[DS18B20](/gekko/fr/reference/devices/ds18b20/) est qu'une thermistance est
*analogique* : elle ne fait que changer de résistance, il faut donc mesurer
cette résistance et la convertir en température. Gekko fait les deux.

Par rapport à un DS18B20, une NTC est moins chère et peut être physiquement
très petite ou très rapide, mais elle est moins précise dès la sortie de
boîte, a besoin d'une résistance et d'un ADC, et la résistance du câble peut
influencer la lecture. Utilisez un DS18B20 pour la précision plug-and-play ;
utilisez une NTC quand vous voulez du pas cher, petit ou rapide — ou si vous
avez déjà un [ADS1115](/gekko/fr/reference/devices/analog-inputs/) avec un canal
libre.

## Câblage : le pont diviseur

On ne lit pas une résistance directement — on lit une tension. La thermistance
est donc mise en série avec une **résistance série** fixe pour former un
diviseur entre l'alimentation et la masse, et Gekko mesure la tension au
point milieu :

![Pont diviseur NTC alimentant une entrée analogique, puis le capteur NTC convertissant les millivolts en température](../../../../../assets/diagrams/ntc-divider.svg)

Quand la résistance de la NTC change avec la température, la tension
milieu change ; Gekko reconvertit cette tension en résistance de la NTC (il
connaît la résistance série et l'alimentation), puis la résistance en
température. Une résistance série de **10 kΩ** associée à une thermistance de
**10 kΩ (à 25 °C)** est la combinaison classique et le défaut de Gekko.

Ce point milieu n'est qu'une tension analogique — donc le capteur NTC ne
possède pas de broche ADC à lui seul. Il dépend d'une
**[entrée analogique](/gekko/fr/reference/devices/analog-inputs/)**, ce qui
signifie que vous pouvez câbler le diviseur vers :

- la **broche ADC native de l'ESP32** (`analog_port_input`) — la plus simple,
  la moins précise ;
- un **canal ADS1115** (`analog_input_channel` sur un `ads1115_hub`) — l'option
  précise, et celle qui rend une thermistance bon marché réellement exploitable ;
- un **canal CD74HC4067** — quand vous avez beaucoup de thermistances
  partageant une seule broche ADC.

## Mise en route

1. Créez l'entrée analogique à laquelle le milieu du diviseur est câblé —
   voir [Entrées analogiques](/gekko/fr/reference/devices/analog-inputs/).
   Un canal ADS1115 est le choix recommandé pour une lecture stable.
2. Créez un **`ntc_thermistor_temperature_sensor`** et sélectionnez cette
   entrée analogique comme dépendance.
3. Choisissez un **preset** correspondant à votre thermistance, ou entrez les
   valeurs à la main.

![Réglages de la NTC : sélecteur d'entrée analogique, preset, valeurs de diviseur, mode de formule et reporting](../../../../../assets/screenshots/device-ntc-thermistor.png)

### Les presets ne sont qu'un raccourci

Le formulaire propose quelques modèles courants de thermistance :

| Preset | Series R | Nominal R (25 °C) | Beta |
| --- | --- | --- | --- |
| Generic 10k B3950 | 10 kΩ | 10 kΩ | 3950 |
| EPCOS/TDK 10k B3435 | 10 kΩ | 10 kΩ | 3435 |
| Vishay 10k B3977 | 10 kΩ | 10 kΩ | 3977 |
| Semitec 100k B4267 | 100 kΩ | 100 kΩ | 4267 |

Un preset ne **pré-remplit que les champs numériques** — rien dans ce choix
n'est stocké sur l'appareil. En choisir un puis ajuster une valeur ensuite
reste toujours sûr ; le preset ne "se bat" jamais contre vos modifications.
Choisissez le plus proche et ajustez, ou sélectionnez *Custom* et saisissez
les valeurs de la fiche technique de votre thermistance.

## Les deux courbes : Beta vs Steinhart-Hart

Convertir une résistance en température nécessite un modèle de courbe de la
thermistance. Gekko propose les deux standards :

- **Équation Beta** — `1/T = 1/T₀ + (1/β)·ln(R/R₀)`. La forme à deux points
  publiée par chaque fiche technique : résistance nominale R₀ à température
  nominale T₀ (généralement 10 kΩ à 25 °C) plus un seul coefficient **Beta**.
  Précise à environ ±0,5–1 °C sur une plage d'aquarium — largement suffisant
  pour un chauffage ou un refroidisseur. C'est le défaut et la forme la plus
  simple à renseigner.
- **Équation Steinhart-Hart** — `1/T = A + B·ln(R) + C·ln(R)³`. Trois
  coefficients au lieu d'un, plus précise sur une plage plus large quand vous
  les connaissez (ou quand vous les ajustez à partir d'un tableau
  résistance/température à 3 points). Ne la choisissez que si vous avez les
  valeurs A/B/C ; sinon Beta est le bon choix.

Basculez entre les deux avec le sélecteur **formula mode** ; le formulaire
affiche les champs nécessaires à l'équation choisie.

## Calibration et lissage

Comme une lecture de thermistance dépend des tolérances (la thermistance elle-même, la résistance série, l'ADC), le capteur profite du conditionnement standard de Gekko :

- un **offset/factor de calibration** pour corriger une erreur connue par
  rapport à un thermomètre de référence ;
- un **poids de lissage** pour amortir le dernier bruit ADC.

Placez un thermomètre de référence à côté de la sonde, lisez les deux, puis
ajustez l'offset jusqu'à concordance — ce trim à un seul point supprime la
plupart des erreurs d'une thermistance bon marché.

## Suivi

Le capteur rapporte sa température avec un indicateur de validité — si son
entrée analogique devient invalide (diviseur débranché, bus I2C malade derrière
un ADS1115), la lecture apparaît comme *invalid*, jamais comme une valeur
figée ou fausse. Cliquez sur sa tuile de tableau de bord pour la valeur en
direct et un graphique d'historique, exactement comme pour le DS18B20.

La température alimente tout le reste dans Gekko comme n'importe quel autre
capteur de température :

- un [thermostat](/gekko/fr/reference/devices/thermostat/) qui pilote un chauffage
  ou un refroidisseur ;
- des [espaces réservés d'affichage](/gekko/fr/guides/displays/) sur tout affichage compatible ;
- Home Assistant comme entité `sensor` en lecture seule sur les
  [builds MQTT](/gekko/fr/guides/mqtt-home-assistant/).

## Configuration

| Field | Default | Meaning |
| --- | --- | --- |
| `formulaMode` | `beta` | `beta` ou `steinhart_hart` |
| `seriesResistorOhms` | `10000` | La résistance fixe du diviseur, en ohms |
| `supplyMilliVolts` | `3300` | Tension d'alimentation du diviseur (rail 3,3 V) |
| `nominalResistanceOhms` | `10000` | Résistance de la thermistance à la température nominale (R₀) |
| `nominalTempCelsius` | `25` | Température nominale (T₀) |
| `betaCoefficient` | `3950` | Valeur Beta (mode Beta) |
| `steinhartA` / `steinhartB` / `steinhartC` | `0` | Coefficients Steinhart-Hart (mode Steinhart-Hart) |
| `unit` | `celsius` | Unité d'affichage |
| `pollMs` | `5000` | Fréquence de lecture |
| `reportDeltaCelsius` | `0.1` | Changement minimal avant envoi d'une nouvelle lecture |
| `reportAlways` | off | Envoie chaque lecture, quel que soit l'écart |

La température évolue lentement — le poll par défaut de 5 s avec un petit
delta de rapport garde le WebSocket et l'historique calmes sans manquer ce qui
compte.

Internals firmware, maths des courbes et table de presets :
[`docs/analog-input.md`](https://github.com/yoreek/gekko/blob/master/docs/analog-input.md).
