---
title: FAQ et dépannage
description: Problèmes courants de flash, provisioning et utilisation d'un contrôleur Gekko — et leurs corrections.
sidebar:
  order: 3
---

## Flashage

### L'installateur web dit que mon navigateur n'est pas pris en charge

Web Serial n'existe que dans les navigateurs Chromium — utilisez **Chrome,
Edge ou Opera sur ordinateur**. Firefox, Safari et tous les navigateurs
mobiles ne peuvent pas flasher. Vous pouvez aussi utiliser les
[scripts esptool](/gekko/fr/getting-started/flashing/), qui fonctionnent partout.

### L'installateur ne liste pas le port série de ma carte

- Utilisez un câble USB **de données** — beaucoup de câbles fournis sont
  uniquement charge.
- Installez le pilote CP210x ou CH340 requis par la puce USB-série de votre
  carte.
- Sous Linux, ajoutez-vous au groupe série (`sudo usermod -a -G dialout
  $USER`, puis reconnectez-vous) et notez que certaines combinaisons Linux +
  Chrome + puce USB sont connues pour être fragiles via Web Serial — le chemin
  esptool reste la solution fiable.
- Fermez tout ce qui tient déjà le port (moniteurs série, IDE).

## Premier démarrage et WiFi

### Le point d'accès de configuration `gekko-…` n'apparaît jamais

- Laissez à la carte environ 10 secondes après la mise sous tension.
- Si l'appareil a déjà été flashé et conserve d'anciens identifiants, il
  passe directement en mode station — vérifiez la liste des clients de votre
  routeur pour trouver son IP.
- Reflashez avec l'option erase (option "erase device" de l'installateur web,
  ou d'abord `esptool erase_flash` avec esptool) pour revenir à un premier
  démarrage propre.

### Je suis connecté à l'AP de configuration mais aucun portail ne s'ouvre

Tous les systèmes n'ouvrent pas automatiquement le portail captif. Ouvrez
vous-même `http://192.168.4.1/` dans un navigateur.

### J'ai enregistré de mauvais identifiants WiFi

Rien n'est perdu : les tentatives de connexion dépendent d'un timeout et le
point d'accès de configuration reste disponible en parallèle. Reconnectez-vous
à l'AP `gekko-…` et corrigez les réglages sur la page WiFi.

## Portail et périphériques

### Le portail charge mais un périphérique affiche `dependency_blocked`

Une de ses dépendances est désactivée, supprimée ou en panne — par ex. un
DS18B20 dont le périphérique bus 1-Wire est désactivé. Corrigez d'abord le
périphérique parent ; l'enfant se rétablit tout seul.

### Mon DS18B20 n'apparaît pas dans le scan du bus

Vérifiez la pull-up d'environ 4,7 kΩ entre DATA et 3,3 V, ainsi que le
câblage. Une sonde saine se scanne avec le code famille `28` et une adresse de
16 caractères, sans drapeau CRC.

### Les programmes n'allument jamais rien

Les programmes exigent une horloge plausible. Réglez le fuseau horaire et NTP
sur la page **Time**, ou ajoutez une RTC DS3231. Rappelez-vous aussi qu'un
interrupteur automatique doit être en mode **Auto** — un override manuel Off/On
ignore les conditions, et un interrupteur automatique sans condition reste
éteint par conception.

### Où sont passées les pages OTA / MQTT ?

Ces pages n'apparaissent que sur les builds firmware compilés avec la
fonctionnalité correspondante — voir
[Mises à jour OTA](/gekko/fr/guides/ota-updates/) et
[MQTT et Home Assistant](/gekko/fr/guides/mqtt-home-assistant/).

## Récupération

### Réinitialisation d'usine

Reflashez avec un effacement complet (option erase de l'installateur web, ou
`esptool erase_flash` + reflash). Cela efface les identifiants WiFi et tout le
registre de périphériques — exportez d'abord une
[sauvegarde](/gekko/fr/guides/backup-restore/) si vous voulez restaurer ensuite.

### Quelle version de firmware est-ce que j'exécute ?

`GET /api/system/version`, la page System du portail, ou la ligne
`Gekko booting version=…` dans le log de démarrage série.
