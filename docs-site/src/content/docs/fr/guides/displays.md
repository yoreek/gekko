---
title: Affichages et concepteur de mise en page
description: Configurez des affichages à pixels, à caractères et sept segments avec le concepteur visuel de Gekko.
sidebar:
  order: 3
---

Gekko prend en charge sept types d'affichage avec un **concepteur visuel de
mise en page** commun. Vous configurez pages et widgets dans le navigateur avec
un aperçu ; les coordonnées et widgets s'adaptent à l'affichage.

| Type | Matériel | Coordonnées | Widgets |
| --- | --- | --- | --- |
| `ssd1306` | OLED I2C monochrome | Pixels | Texte, formes et bitmaps |
| `st7735` | TFT SPI couleur | Pixels | Texte, formes et bitmaps RGB565 |
| `lcd1602` | HD44780 16 × 2 via PCF857x | Cellules de caractères | Character |
| `lcd2004` | HD44780 20 × 4 via PCF857x | Cellules de caractères | Character |
| `lcd1602_pin` | HD44780 16 × 2 câblé directement sur des broches GPIO | Cellules de caractères | Character |
| `lcd2004_pin` | HD44780 20 × 4 câblé directement sur des broches GPIO | Cellules de caractères | Character |
| `tm1637` | Module sept segments à quatre chiffres | Positions de chiffres | Digital |

## Configuration d'un affichage

1. Créez d'abord les périphériques nécessaires :
   - un [**bus I2C**](/gekko/fr/reference/devices/i2c-bus/) pour SSD1306 ;
   - un [**bus SPI**](/gekko/fr/reference/devices/spi-bus/) pour ST7735 ;
   - un [**expanseur de ports**](/gekko/fr/reference/devices/port-expanders/)
     PCF8574/PCF8575 pour LCD1602/LCD2004 ;
   - rien pour le TM1637 : il pilote directement ses broches CLK et DIO ;
   - rien non plus pour LCD1602_PIN/LCD2004_PIN : ils possèdent directement
     leurs 6 broches de données/contrôle, sans module I2C ni expanseur de
     ports — choisissez ces variantes si vous préférez câbler le LCD
     directement à l'ESP32 plutôt que d'ajouter un expanseur PCF857x.
2. Créez l'affichage, sélectionnez ces périphériques comme dépendances, puis
   configurez adresse, câblage, broches de contrôle, luminosité ou rotation.
   LCD1602_PIN/LCD2004_PIN demandent à la place directement leurs broches
   `rs`, `e`, `d4`–`d7` et, en option, `backlight`.
3. Ouvrez le périphérique et cliquez sur **Design** pour entrer dans le
   concepteur de mise en page.

![Concepteur de mise en page d'affichage](../../../../assets/screenshots/portal-display-designer.png)

## Pages et widgets

Une mise en page contient des **pages** et des **widgets** positionnés. Les
affichages à pixels utilisent des pixels, LCD1602/LCD2004 des cellules de
caractères et TM1637 des positions de chiffres. Le concepteur autorise
uniquement les widgets compatibles et affiche un aperçu adapté. Les mises en
page sont enregistrées sur l'appareil et incluses dans les
[lots de sauvegarde](/gekko/fr/guides/backup-restore/).

## Valeurs en direct : espaces réservés de métriques

Les widgets Text, Character et Digital peuvent mélanger du texte statique avec
des **espaces réservés** résolus au moment du rendu.

![Texte du widget avec espaces réservés à gauche, sortie OLED rendue avec valeurs en direct à droite](../../../../assets/diagrams/display-placeholders.svg)

Vous n'avez pas besoin de mémoriser la syntaxe — le **générateur d'espaces
réservés** du concepteur liste toutes les métriques disponibles de chaque
périphérique avec une valeur d'aperçu en direct, et insère l'espace réservé
pour vous. Les espaces réservés typés sont validés à chaque frappe. Quelques
exemples :

```text
Room {{dev.670845748.temperature | fixed:1}}
IP {{system.wifi.station_ip}}
Now {{system.time | format:HH:mm}}
Up {{system.uptime}}
```

Formes d'espaces réservés :

- `{{dev.<deviceId>.<metricKey>}}` — une métrique de n'importe quel
  périphérique (température, état, …). Le concepteur dispose d'un générateur
  d'espaces réservés listant tout ce qui est disponible.
- `{{system.<metricKey>}}` — des métriques système telles que `time` (horloge
  murale) et `uptime` (temps écoulé depuis le démarrage).
- `{{system.wifi.<metricKey>}}` — des métriques WiFi telles que `station_ip`.

Les filtres optionnels viennent ensuite après `|` :

| Filter | Example | Effect |
| --- | --- | --- |
| `fixed:N` | `{{dev.123.temperature \| fixed:1}}` | Formatage décimal avec N chiffres |
| `format:pattern` | `{{system.time \| format:EEEE HH:mm}}` | Modèle date/heure (`YYYY MM DD HH mm ss EEEE`; texte `[literal]` entre crochets) |
| `upper` / `lower` / `trim` | `{{system.wifi.station_ip \| upper}}` | Transformations de texte |

Un espace réservé impossible à résoudre est rendu comme `N/A` au lieu de
casser tout le widget, donc un capteur temporairement absent ne vide jamais
votre écran — il affiche `N/A` à cet endroit.

Les périphériques référencés par les espaces réservés deviennent de vraies
dépendances du registre pour l'affichage — le registre vous avertira avant de
supprimer un capteur encore affiché.
