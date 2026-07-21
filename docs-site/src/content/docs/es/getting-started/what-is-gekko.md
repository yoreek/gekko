---
title: ¿Qué es Gekko?
description: Una introducción a Gekko, un controlador modular de dispositivos para ESP32 con un portal web integrado.
sidebar:
  order: 1
---

Gekko es firmware para el ESP32 más un portal web servido directamente desde
la memoria flash del propio dispositivo. Juntos te permiten construir tu
propio controlador - acuario, terrario, invernadero o automatización del
hogar general - a partir de un catálogo de tipos de dispositivo, cableados y
configurados por completo desde la interfaz.

**Una sola imagen de firmware, sin recompilar por proyecto.** Cada tipo de
dispositivo compatible ya viene incorporado. Añadir un relé, un sensor de
temperatura, una pantalla o una bomba dosificadora es una acción del portal
sobre el dispositivo en ejecución, nunca una recompilación.

## Qué puedes construir con él

- **Interruptores y salidas** - relés GPIO, interruptores detrás de
  expansores I2C PCF8574/PCF8575, salidas PWM/analógicas con transiciones
  suaves, curvas diarias de brillo y agrupaciones multicanal.
- **Sensores** - sondas de temperatura DS18B20 (1-Wire) y termistores NTC,
  HTU21 para temperatura + humedad y entradas binarias digitales (contactos
  de puerta, flotadores, sensores de fuga).
- **Automatización** - horarios diarios con precisión de minuto, auto-switches
  guiados por condiciones con anulación manual y pausa, termostatos con
  histéresis y bombas dosificadoras con calibración y diario de dosis.
- **Pantallas** - OLED SSD1306 y TFT ST7735 con un diseñador visual de
  páginas/widgets integrado en el portal, incluyendo marcadores vivos como
  `{{dev.123.temperature}}`.
- **Infraestructura** - buses I2C/SPI/1-Wire, un reloj en tiempo real DS3231 y
  un panel que compones a partir de tarjetas.

Los dispositivos declaran **dependencias** entre sí - un interruptor sobre un
expansor de puertos, un sensor sobre un bus I2C, una bomba gobernada por un
horario - y el registro valida, aplica y persiste ese grafo. Consulta
[Dispositivos y dependencias](/gekko/es/guides/devices-and-dependencies/) para el
concepto.

## Qué hace diferente a Gekko

**No hay una imagen de firmware por configuración.** Muchos firmwares de
controladores convierten tu configuración en una compilación dedicada - añadir
un sensor implica editar un archivo de configuración, recompilar y reflashear.
Gekko entrega una sola imagen con todos los tipos de dispositivo compatibles
ya incluidos; cambiar la instalación es siempre una acción del portal sobre el
dispositivo en ejecución, nunca una recompilación.

**Estructura en vez de plantillas de pines.** Configurar en tiempo de ejecución
suele significar una lista plana de GPIOs y reglas de consola. Gekko modela el
hardware como realmente está cableado: un registro tipado de dispositivos con
dependencias declaradas, cada uno con su propia configuración versionada que se
migra automáticamente entre versiones de firmware.

**Todo es observable y programable.** El estado vivo viaja por WebSocket,
cada dispositivo habla la misma API REST, los eventos importantes van al
diario, las pantallas tienen diseñador visual y el panel se compone de
tarjetas, no de una sola pantalla de consola.

**Home Assistant con un solo interruptor.** En compilaciones con MQTT, publicar
un dispositivo en Home Assistant es un solo interruptor en su página: aparece
allí como una entidad nativa (switch, sensor, climate) que puedes controlar
desde HA, mientras todo sigue funcionando localmente. Consulta
[MQTT y Home Assistant](/gekko/es/guides/mqtt-home-assistant/).

La compensación honesta: el catálogo de tipos de dispositivo está fijado en
tiempo de compilación, así que es deliberadamente una base más pequeña y más
estructurada que un catálogo de absolutamente cualquier sensor.

## Todo corre en el dispositivo

- **Local-first** - el portal se sirve desde el ESP32 por WiFi; sin nube, sin
  cuenta, sin tienda de apps.
- **Integraciones opcionales** - MQTT + descubrimiento de Home Assistant y
  actualizaciones OTA existen, pero vienen desactivadas por defecto.
- **Provisionamiento WiFi** - un punto de acceso de configuración (o BLE, si
  está habilitado) conecta el dispositivo a tu red sin credenciales fijas.
- **Copia de seguridad y restauración** - toda la configuración del dispositivo
  se exporta como un solo paquete editable por humanos.
- **Siete idiomas** - el portal detecta automáticamente el idioma del
  navegador y está disponible en inglés, ucraniano, ruso, alemán, español,
  francés e italiano.

## Próximos pasos

1. [Comprobar los requisitos de hardware](/gekko/es/getting-started/hardware/)
2. [Flashear el firmware](/gekko/es/getting-started/flashing/) - desde el
   navegador o con esptool/PlatformIO
3. [Conectar el dispositivo al WiFi](/gekko/es/getting-started/first-boot-wifi/)
4. [Añadir tu primer dispositivo](/gekko/es/getting-started/first-device/)
