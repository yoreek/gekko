---
title: Pantallas y diseñador de disposición
description: Configura pantallas de píxeles, caracteres y siete segmentos con el diseñador visual de Gekko.
sidebar:
  order: 3
---

Gekko admite cinco tipos de pantalla mediante un **diseñador visual de
disposición** compartido. Configuras páginas y widgets en el navegador con
vista previa; las coordenadas y widgets se adaptan a la pantalla.

| Tipo | Hardware | Coordenadas | Widgets |
| --- | --- | --- | --- |
| `ssd1306` | OLED I2C monocromo | Píxeles | Texto, formas y bitmaps |
| `st7735` | TFT SPI a color | Píxeles | Texto, formas y bitmaps RGB565 |
| `lcd1602` | HD44780 16 × 2 mediante PCF857x | Celdas de caracteres | Character |
| `lcd2004` | HD44780 20 × 4 mediante PCF857x | Celdas de caracteres | Character |
| `tm1637` | Módulo de siete segmentos de cuatro dígitos | Posiciones de dígitos | Digital |

## Configurar una pantalla

1. Crea primero los dispositivos necesarios:
   - un [**bus I2C**](/gekko/es/reference/devices/i2c-bus/) para SSD1306;
   - un [**bus SPI**](/gekko/es/reference/devices/spi-bus/) para ST7735;
   - un [**expansor de puertos**](/gekko/es/reference/devices/port-expanders/)
     PCF8574/PCF8575 para LCD1602/LCD2004;
   - nada para TM1637: controla sus pines CLK y DIO directamente.
2. Crea la pantalla, selecciona esos dispositivos como dependencias y
   configura dirección, cableado, pines de control, brillo o rotación.
3. Abre el dispositivo y haz clic en **Design** para entrar en el diseñador de
   disposición.

![Diseñador de disposición de pantalla](../../../../assets/screenshots/portal-display-designer.png)

## Páginas y widgets

Una disposición contiene **páginas** con **widgets** posicionados. Las
pantallas de píxeles usan píxeles, LCD1602/LCD2004 usan celdas de caracteres y
TM1637 usa posiciones de dígitos. El diseñador permite solo los widgets
compatibles y muestra una vista previa adecuada. Las disposiciones se guardan
en el dispositivo y se incluyen en los
[paquetes de copia de seguridad](/gekko/es/guides/backup-restore/).

## Valores en vivo: marcadores de métricas

Los widgets Text, Character y Digital pueden mezclar texto estático con
**marcadores** resueltos en tiempo de renderizado.

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
