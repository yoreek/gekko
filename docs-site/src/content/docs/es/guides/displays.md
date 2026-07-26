---
title: Pantallas y diseñador de disposición
description: Controla pantallas OLED SSD1306 y TFT ST7735 con el diseñador visual de disposición de Gekko y marcadores vivos de métricas.
sidebar:
  order: 3
---

Gekko controla pantallas OLED I2C **SSD1306** y pantallas TFT SPI **ST7735**,
y el portal incluye un **diseñador visual de disposición**: compones lo que
debe mostrar la pantalla a partir de páginas y widgets, en vivo en el
navegador, con vista previa.

## Configurar una pantalla

1. Crea primero el dispositivo de bus: un
   [**bus I2C**](/gekko/es/reference/devices/i2c-bus/) (pines SDA/SCL) para
   SSD1306, o un [**bus SPI**](/gekko/es/reference/devices/spi-bus/) para ST7735.
2. Crea el dispositivo de pantalla y selecciona ese bus como dependencia
   (además de la dirección I2C o los pines de control del TFT).
3. Abre el dispositivo y haz clic en **Design** para entrar en el diseñador de
   disposición.

![Diseñador de disposición de pantalla](../../../../assets/screenshots/portal-display-designer.png)

## Páginas y widgets

Una disposición es un conjunto de **páginas**; cada página contiene
**widgets** posicionados (texto y más). El diseñador muestra una vista previa
en vivo renderizada con las mismas fuentes y métricas que usa el firmware, así
que lo que ves es lo que dibuja el panel. Las disposiciones se guardan en el
dispositivo y se incluyen en los
[paquetes de copia de seguridad](/gekko/es/guides/backup-restore/).

## Valores en vivo: marcadores de métricas

Los widgets de texto pueden mezclar texto estático con **marcadores** resueltos
en tiempo de renderizado. Crear una pantalla de estado es solo escribir unas
líneas de texto plantilla:

![Texto de widget con marcadores a la izquierda, salida OLED renderizada con valores vivos a la derecha](../../../../assets/diagrams/display-placeholders.svg)

No hace falta memorizar la sintaxis: el **constructor de marcadores** del
diseñador lista cada métrica disponible de cada dispositivo con un valor de
vista previa en vivo e inserta el marcador por ti. Los marcadores tipados se
validan en cada pulsación. Más ejemplos:

```text
Room {{dev.670845748.temperature | fixed:1}}
IP {{system.wifi.station_ip}}
Now {{system.time | format:HH:mm}}
Up {{system.uptime}}
```

Formas de marcador:

- `{{dev.<deviceId>.<metricKey>}}` - una métrica de cualquier dispositivo
  (temperatura, estado, ...). El diseñador tiene un constructor que lista todo
  lo disponible.
- `{{system.<metricKey>}}` - métricas del sistema como `time` (reloj) y
  `uptime` (tiempo desde el arranque).
- `{{system.wifi.<metricKey>}}` - métricas WiFi como `station_ip`.

Los filtros opcionales van después de `|`:

| Filtro | Ejemplo | Efecto |
| --- | --- | --- |
| `fixed:N` | `{{dev.123.temperature \| fixed:1}}` | Formato decimal con N dígitos |
| `format:pattern` | `{{system.time \| format:EEEE HH:mm}}` | Patrón de fecha/hora (`YYYY MM DD HH mm ss EEEE`; texto `[literal]` entre corchetes) |
| `upper` / `lower` / `trim` | `{{system.wifi.station_ip \| upper}}` | Transformaciones de texto |

Un marcador que no pueda resolverse se renderiza como `N/A` en vez de romper
todo el widget, así que un sensor temporalmente ausente no deja la pantalla
en blanco — mostrará `N/A` en su lugar.

Los dispositivos referenciados por marcadores se convierten en dependencias
reales del registro para esa pantalla: el registro te avisará antes de borrar
un sensor que una pantalla todavía muestra.
