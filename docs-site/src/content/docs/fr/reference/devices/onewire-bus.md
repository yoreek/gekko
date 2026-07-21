---
title: Bus 1-Wire
description: Comment fonctionne le bus 1-Wire — câblage, résistance de rappel, adresses ROM 64 bits et nombre de capteurs partageant une seule broche ESP32.
sidebar:
  order: 3
  label: Bus 1-Wire
---

## Qu'est-ce que le 1-Wire ?

Le 1-Wire est un bus série (à l'origine de Dallas Semiconductor) construit
autour d'une idée radicale : **une seule ligne de données partagée pour tout**.
Le contrôleur et n'importe quel nombre de périphériques — en pratique, des
sondes de température DS18B20 — partagent le même fil, et chaque périphérique
est adressé individuellement. C'est pourquoi tout un aquarium de sondes de
température ne consomme exactement **qu'une broche GPIO**.

Dans Gekko, le bus lui-même est un périphérique : `onewire_bus`. Il possède la
broche, lance le scan, et chaque [capteur DS18B20](/gekko/reference/devices/ds18b20/)
créé en dépend.

## Câblage : trois fils et une résistance

![Câblage 1-Wire : 3V3, GND, DATA avec une pull-up de 4,7 kΩ entre DATA et 3V3](../../../../assets/diagrams/onewire-wiring.svg)

Une sonde DS18B20 étanche typique a trois fils :

| Fil | Couleur habituelle | À connecter à |
| --- | --- | --- |
| VDD | rouge | 3,3 V |
| DATA | jaune (ou blanc/bleu) | la broche GPIO du bus |
| GND | noir | GND |

La **résistance pull-up de 4,7 kΩ** entre DATA et 3,3 V n'est pas
optionnelle. La ligne de données est *open-drain* : aucun périphérique ne la
pousse jamais à l'état haut — ils la tirent seulement vers le bas puis la
relâchent. La résistance ramène la ligne à 3,3 V entre les bits ; sans elle,
toute lecture est du bruit. Une seule résistance par bus, quel que soit le
nombre de sondes.

:::tip
La configuration `onewire_bus` comporte une option **internal pull-up** qui
utilise la faible résistance interne ~45 kΩ de l'ESP32. Elle peut fonctionner
pour une seule sonde sur un câble très court, mais elle est bien plus faible
que la 4,7 kΩ recommandée — au-delà d'une breadboard, mettez la vraie
résistance.
:::

Les sondes vendues « avec adaptateur/module » ont souvent déjà la résistance
sur la petite carte — n'en ajoutez pas une seconde.

## Plusieurs périphériques sur un même fil

![Topologie du bus : ESP32 avec pull-up et trois sondes sur une seule ligne de données, chacune avec sa propre adresse ROM](../../../../assets/diagrams/onewire-bus.svg)

Les nouvelles sondes se câblent **en parallèle** sur les trois mêmes lignes —
prenez DATA, 3,3 V et GND là où c'est pratique. Un chaînage en ligne
(sonde vers sonde le long d'un câble) est électriquement le plus propre ;
des dérivations courtes sur une ligne principale conviennent aussi. Des
longueurs de plusieurs mètres sont courantes avec la pull-up 4,7 kΩ.

## Adresses : comment les sondes évitent de se percuter

Chaque périphérique 1-Wire est identifié par une **adresse ROM 64 bits**
unique :

![Anatomie d'une adresse ROM 64 bits : code famille 8 bits, série unique 48 bits, CRC 8 bits ; le scan découvre, les adresses matchent un périphérique à la fois](../../../../assets/diagrams/onewire-rom.svg)

Deux opérations rendent le fil partagé utilisable :

- **Search** (« scan » dans le portail) — une élimination binaire astucieuse
  qui découvre toutes les adresses sur le bus sans en connaître aucune à
  l'avance. Le périphérique bus de Gekko expose cela comme une commande de
  **scan** ; les résultats (code famille, adresse, état CRC) alimentent
  directement la boîte de dialogue de création DS18B20.
- **Match** — pour parler à un seul périphérique, le contrôleur envoie
  d'abord son adresse complète ; seul ce périphérique répond. Les
  périphériques ne parlent jamais de leur propre initiative, donc il n'y a
  pas de collision — le contrôleur interroge toujours.

Le premier octet est le **code famille** — `28` signifie « capteur de
température DS18B20 ». Le scan rapporte tout ce qu'il trouve, et Gekko filtre
les candidats DS18B20 par ce code.

## Configuration

| Field | Default | Meaning |
| --- | --- | --- |
| `gpioPin` | `4` | La broche de données du bus |
| `internalPullup` | off | Utiliser la pull-up interne faible de l'ESP32 au lieu d'une pull-up externe de 4,7 kΩ |
| `enabled` | on | Désactiver le bus bloque tous les capteurs qui en dépendent (`dependency_blocked`) |

## Dépannage

- **Le scan ne trouve rien** — vérifiez d'abord la pull-up, puis l'ordre du
  câblage (inverser VDD et DATA est l'erreur classique ; les sondes y
  survivent).
- **La sonde apparaît avec un drapeau CRC** — mauvais contact ou
  interférences ; raccourcissez la ligne, améliorez les connexions, éloignez-la
  des câbles secteur et des alimentations à découpage.
- **Deux sondes, une seule adresse affichée** — vous avez scanné avant de
  connecter la seconde sonde ; relancez le scan.
