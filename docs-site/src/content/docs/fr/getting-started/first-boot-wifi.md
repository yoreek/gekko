---
title: Premier démarrage et configuration WiFi
description: Connectez un contrôleur Gekko fraîchement flashé à votre réseau WiFi via son point d'accès de configuration.
sidebar:
  order: 4
---

Gekko est livré **sans identifiants WiFi codés en dur**. Au premier démarrage,
l'appareil ouvre son propre point d'accès de configuration, et vous configurez
votre réseau depuis le portail.

## Connexion via le point d'accès de configuration

1. Allumez la carte fraîchement flashée. En quelques secondes, elle démarre un
   point d'accès WiFi ouvert nommé **`gekko-<suffix>`**, où le suffixe vient de
   l'adresse MAC de la carte — ainsi deux contrôleurs côte à côte n'entrent
   jamais en collision.
2. Connectez-vous à ce point d'accès depuis un téléphone ou un ordinateur
   portable. Sur la plupart des systèmes, une invite de portail captif
   apparaît ; sinon, ouvrez le portail directement par IP —
   `http://192.168.4.1/` (l'adresse AP ESP32 par défaut).
3. Ouvrez la page **WiFi** dans le portail. L'appareil recherche les réseaux
   à proximité et en affiche la liste.
4. Choisissez votre réseau, saisissez le mot de passe et enregistrez.
5. L'appareil se connecte à votre réseau en mode station. Le point d'accès de
   configuration est géré par la machine d'état WiFi — il reste disponible
   jusqu'à ce que la connexion station soit établie, donc une faute de frappe
   dans le mot de passe ne vous bloque jamais.

Après une connexion réussie, ouvrez le portail à l'adresse attribuée à
l'appareil par votre routeur (vérifiez la liste des clients du routeur ou la
ligne de log série de l'appareil). À partir de là, le portail est servi sur
votre réseau normal.

## Si la connexion échoue

Des identifiants enregistrés pour un réseau inaccessible ne **briquent** pas
l'appareil : les tentatives de connexion station sont pilotées par timeout, et
le point d'accès de configuration ainsi que le portail restent disponibles tout
le temps — reconnectez-vous à l'AP et corrigez les réglages.

## Alternative : provisionnement BLE

Les builds avec provisionnement mobile activé peuvent aussi recevoir les
identifiants WiFi via **Bluetooth LE** avec une application de provisioning
compatible Espressif (Android/iOS). Le mode de config BLE ne démarre qu'après
une demande explicite depuis le portail ou l'API, fonctionne avec un timeout de
session, et ne modifie jamais les identifiants stockés tant que l'application
n'envoie pas de nouveaux paramètres avec succès. Si vous avez flashé l'image
par défaut, utilisez le flux AP de configuration ci-dessus — il est toujours
disponible.

## Suite

Une fois l'appareil sur votre réseau, faites le
[tour du portail](/gekko/getting-started/portal-tour/) ou passez directement à
[l'ajout de votre premier périphérique](/gekko/getting-started/first-device/).
