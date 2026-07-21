---
title: Mises à jour OTA
description: Mises à jour du firmware par liaison radio dans Gekko — ce qui fonctionne sur les cartes 4 Mo et ce qui demande plus de flash.
sidebar:
  order: 6
---

Gekko dispose de deux chemins de mise à jour sans fil, tous deux **désactivés
dans la build par défaut** — la carte ESP32 standard de 4 Mo n'a tout
simplement pas la marge de flash nécessaire pour un schéma de partition OTA à
côté du firmware, du portail web et de votre configuration.

## Build par défaut : mise à jour par série

Sur la disposition standard à une seule application, les mises à jour sont un
[reflash USB](/gekko/getting-started/flashing/). Votre configuration
d'appareil est stockée dans NVS et survit à un reflash firmware — pensez tout
de même à garder un
[lot de sauvegarde](/gekko/guides/backup-restore/) avant de mettre à jour.

## Upload OTA via PlatformIO (développeurs)

Pour les cartes avec suffisamment de flash et une disposition de partitions
activant l'OTA, l'environnement PlatformIO `esp32dev_ota` livre la même image
firmware sur le réseau plutôt que par série :

```sh
pio run -e esp32dev_ota -t upload
```

C'est volontairement un alias de transport d'upload de `esp32dev` — même
image, mêmes flags de build — afin que les livraisons série et OTA restent
identiques octet par octet.

## Web OTA (upload depuis le portail)

Le firmware compilé avec l'option Web OTA protégée ajoute une page **OTA** au
portail : chargez une image firmware depuis le navigateur, et l'appareil la
vérifie, la finalise et redémarre dessus. Les uploads trop gros ou interrompus
laissent le firmware en cours intact. Sur les builds sans cette fonctionnalité,
le portail masque simplement la page OTA.

:::note
Considérez Web OTA comme une fonctionnalité de développement / avancée :
activez-la seulement sur des cartes avec marge de flash, selon
`docs/platformio-environments.md` dans le dépôt.
:::
