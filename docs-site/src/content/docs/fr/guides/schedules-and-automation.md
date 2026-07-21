---
title: Programmes et automatisation
description: Programmes horaires et interrupteurs automatiques pilotés par conditions dans Gekko.
sidebar:
  order: 2
---

Deux types de périphériques travaillent ensemble pour automatiser les
commutations : un **programme** contient des règles d'heure du jour et indique
s'il est actif maintenant, et un **interrupteur automatique** pilote un vrai
interrupteur à partir du ET logique de ses conditions attachées.

## Programme

Un [périphérique programme](/gekko/reference/devices/schedule/) contient
jusqu'à 4 règles. Chaque règle a :

- un **masque de jours de semaine** — quels jours elle s'applique ;
- une **fenêtre horaire** — minute de début et de fin de la journée
  (précision minute, pas de secondes) ;
- un **mode** :
  - **Always on** — actif sur toute la fenêtre ;
  - **Interval** — découpe la fenêtre en tranches égales et reste actif pendant
    les N premières minutes de chaque tranche (pour une circulation périodique,
    la brumisation, etc.).

Le programme est actif quand **n'importe quelle** règle activée correspond.
L'éditeur de règles du portail montre un aperçu client on/off calculé à partir
des règles et de l'horloge de votre navigateur — indiqué comme estimation,
car l'appareil évalue les règles avec sa propre horloge et son propre fuseau.

:::note[Donnez à l'appareil une horloge fiable]
Les programmes refusent d'agir tant que l'horloge de l'appareil n'est pas
plausible. Utilisez NTP (réglez le fuseau sur la page **Time**) ou ajoutez un
périphérique RTC DS3231 pour que les programmes survivent aux coupures
internet et aux redémarrages.
:::

## Interrupteur automatique

Un interrupteur automatique enveloppe un vrai interrupteur (GPIO ou
expanseur de ports) et le pilote à partir de jusqu'à **6 dépendances de
condition** — programmes, autres interrupteurs, ou autres interrupteurs
automatiques — chacune pouvant être **inversée**. Toutes les conditions sont
reliées par un ET : la sortie est activée seulement lorsque chaque condition
est satisfaite. Sans condition attachée, un interrupteur automatique en mode
Auto reste éteint.

Ses modes sont :

- **Off / On** — override manuel ; les conditions sont ignorées. Le basculer
  depuis le tableau de bord règle exactement cela.
- **Auto** — suivre les conditions.
- **Paused** — temporairement éteint pendant une durée configurée, puis retour
  automatique à **Auto**. La pause n'est disponible qu'à partir du mode Auto.
  Un redémarrage en pleine pause reprend la pause avec le temps restant
  correct.

En entrant en Auto (ou Paused), l'interrupteur cible est toujours forcé à
off d'abord, puis remis aux conditions — donc un ancien "On" manuel ne reste
jamais silencieusement actif.

Comme un interrupteur automatique agit lui-même comme un interrupteur et une
condition, vous pouvez chaîner les automatisations : un interrupteur
automatique "feeding mode" peut bloquer plusieurs autres interrupteurs
automatiques via leurs emplacements de condition inversée.

## Exemple : éclairage d'aquarium avec bouton pause

1. Créez un **Programme** "Light hours", règle : tous les jours, 09:00–21:00,
   toujours actif.
2. Créez un **GPIO Switch** "Light relay" sur la broche qui pilote votre
   éclairage.
3. Créez un **Auto Switch** "Light" : cible = "Light relay", condition =
   "Light hours", mode = Auto.
4. Épinglez "Light" au tableau de bord. Il suit maintenant le programme ;
   tapez dessus pour un override manuel, utilisez **pause** pendant la
   maintenance, puis revenez en Auto quand vous avez fini.

## Périphériques liés

- **[Thermostat](/gekko/reference/devices/thermostat/)** — contrôle de
  température à hystérésis pilotant un interrupteur.
- **[Dosing pump](/gekko/reference/devices/dosing-pump/)** — dosage planifié
  avec calibration, suivi de conteneur et journal des doses.
- **[Scheduled analog output](/gekko/reference/devices/analog-outputs/)** —
  courbe quotidienne de luminosité/niveau pour sorties PWM, composable en
  ensembles multi-canaux.
