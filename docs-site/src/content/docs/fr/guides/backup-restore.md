---
title: Sauvegarde et restauration
description: Exportez toute la configuration d'un appareil Gekko dans un seul lot éditable à la main et restaurez-le partout.
sidebar:
  order: 5
---

Toute la configuration de votre appareil — chaque périphérique, sa
configuration, le graphe de dépendances, les mises en page d'affichage et le
tableau de bord — s'exporte dans **un seul fichier** que vous pouvez
télécharger, conserver, éditer à la main et restaurer sur le même contrôleur ou
sur un autre.

## Depuis le portail

Ouvrez **System → Backup** :

- **Download** enregistre `device-setup.ndjson`.
- **Restore** envoie un lot avec confirmation. Le lot est d'abord validé ;
  toute erreur rejette l'import **sans toucher à la configuration en cours**,
  et une restauration réussie remplace atomiquement tous les périphériques.

## Depuis la ligne de commande

```sh
# backup
curl -fsS http://<device-ip>/api/device-setup/export -o device-setup.ndjson

# restore
curl -fsS -F "bundle=@device-setup.ndjson" http://<device-ip>/api/device-setup/import
```

## Le lot est du JSON brut — et éditable à la main

Le fichier est du NDJSON : un objet JSON par ligne, dans la même forme que
l'API REST accepte. Vous pouvez ajuster un numéro de broche dans un éditeur de
texte, ou même écrire un lot minimal à partir de zéro :

```json
{"kind":"transfer_envelope","transferSchemaVersion":3}
{"kind":"device","record":{"id":4,"typeName":"gpio_switch"},"config":{"name":"Pump","enabled":true,"gpioPin":26}}
```

Seuls `record.id` et `record.typeName` sont requis par périphérique — tout le
reste reçoit des valeurs par défaut. Les ids de dépendance référencent d'autres
`record.id` du même lot, et les lots issus de firmwares plus anciens
s'importent proprement tant que leurs champs restent parsables. Détails du
format complet : [`docs/backup-and-restore.md`](https://github.com/yoreek/gekko/blob/master/docs/backup-and-restore.md).

## Ce qui est inclus — et ce qui ne l'est pas

**Inclus :** le registre des périphériques (tous les types, dépendances,
dispositions d'affichage) et la disposition du tableau de bord.

**Non inclus :** les identifiants WiFi, les paramètres MQTT, l'état retenu de
l'exécution et l'état du firmware/OTA.

## Sauvegardes automatiques

Le firmware n'a volontairement pas de planificateur intégré — l'endpoint
d'export est un simple GET que n'importe quelle machine de votre LAN peut
appeler. Exemple de cron quotidien :

```sh
# /etc/cron.d/gekko-backup — tous les jours à 03:00, conservation 30 jours
0 3 * * * user curl -fsS http://192.168.1.240/api/device-setup/export \
  -o /var/backups/gekko/device-setup-$(date +\%F).ndjson \
  && find /var/backups/gekko -name 'device-setup-*.ndjson' -mtime +30 -delete
```

Ou une automation Home Assistant avec `shell_command` + déclencheur temporel.
Voir le document du dépôt ci-dessus pour un exemple HA prêt à l'emploi.

:::caution
Le portail n'a pas d'authentification — n'importe qui sur votre réseau peut
lire le lot (et le portail). Les configs ne contiennent aucun secret WiFi/MQTT,
mais gardez le contrôleur sur un réseau de confiance.
:::
