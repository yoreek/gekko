---
title: Comment fonctionne Gekko
description: Comprendre le câblage, les dépendances et le flux de contrôle avant de créer des périphériques.
sidebar:
  order: 0
---

Gekko représente le matériel comme un graphe de périphériques composables. Il faut distinguer le câblage physique, les dépendances du registre et le flux de contrôle à l’exécution.

## 1. Câblage physique

Un DS18B20 se branche à une ligne 1-Wire et un relais à une sortie GPIO. Le chauffage est relié au relais, jamais directement à l’ESP32.

## 2. Dépendances des périphériques

Un capteur dépend de son bus ; un thermostat dépend d’un capteur de température et d’un interrupteur compatibles.

![Graphe du thermostat : bus 1-Wire, DS18B20, interrupteur GPIO et thermostat.](../../../../assets/diagrams/fr/thermostat-project-flow.svg)

Gekko valide les rôles lors de la création. Sans dépendance, l’appareil indique `dependency_blocked`.

## 3. Flux de contrôle

```text
Température DS18B20 → thermostat → commande On/Off → relais → chauffage
```

Créez d’abord les bus et sorties, puis les capteurs, attendez `ready`, créez les automatismes et testez l’état sûr. Voir [Périphériques et dépendances](/gekko/fr/guides/devices-and-dependencies/) et [Thermostat avec relais](/gekko/fr/projects/thermostat-with-relay/).
