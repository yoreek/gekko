---
title: FAQ y solución de problemas
description: Problemas comunes al flashear, aprovisionar y ejecutar un controlador Gekko, y cómo resolverlos.
sidebar:
  order: 3
---

## Flasheo

### El instalador web dice que mi navegador no es compatible

Web Serial solo existe en navegadores Chromium: usa **Chrome, Edge u Opera en
escritorio**. Firefox, Safari y todos los navegadores móviles no pueden
flashear. Como alternativa usa los [scripts de esptool](/gekko/es/getting-started/flashing/),
que funcionan en cualquier sitio.

### El instalador no lista el puerto serie de mi placa

- Usa un cable USB de **datos**: muchos cables incluidos solo sirven para
  carga.
- Instala el controlador CP210x o CH340 que necesite el chip USB-serie de tu
  placa.
- En Linux, añade tu usuario al grupo de serie (`sudo usermod -a -G dialout
  $USER`, luego vuelve a iniciar sesión) y ten en cuenta que algunas
  combinaciones Linux + Chrome + chip USB son conocidas por ser inestables con
  Web Serial: la ruta de esptool es el fallback fiable.
- Cierra cualquier otra cosa que esté ocupando el puerto (monitores serie,
  IDEs).

## Primer arranque y WiFi

### El punto de acceso de configuración `gekko-…` nunca aparece

- Dale unos 10 segundos a la placa tras encenderla.
- Si el dispositivo ya se había flasheado antes y conserva credenciales viejas,
  irá directo al modo estación: mira la lista de clientes de tu router para ver
  su IP.
- Reflashea con la opción de borrado (el instalador web ofrece "erase device";
  con esptool, primero `esptool erase_flash`) para volver a un primer arranque
  limpio.

### Estoy conectado al AP de configuración pero no se abre el portal

No todos los sistemas abren automáticamente el portal cautivo. Abre
`http://192.168.4.1/` tú mismo en el navegador.

### Guardé credenciales WiFi incorrectas

No se pierde nada: los reintentos de conexión dependen de timeouts y el AP de
configuración sigue disponible junto a ellos. Reconéctate al AP `gekko-…` y
corrige los ajustes en la página WiFi.

## Portal y dispositivos

### El portal carga, pero un dispositivo muestra `dependency_blocked`

Una de sus dependencias está deshabilitada, borrada o en fallo: por ejemplo,
un DS18B20 cuyo dispositivo de bus 1-Wire está deshabilitado. Repara primero
el dispositivo padre; el hijo se recupera solo.

### Mi DS18B20 no aparece en el escaneo del bus

Revisa la pull-up de ~4,7 kΩ entre data y 3,3 V, y el cableado. Una sonda sana
escanea con código de familia `28` y una dirección de 16 caracteres, sin
bandera CRC.

### Los horarios nunca encienden nada

Los horarios necesitan un reloj plausible. Ajusta la zona horaria y NTP en la
página **Time**, o añade un RTC DS3231. Recuerda también que un auto switch
debe estar en modo **Auto**: una anulación manual Off/On ignora las
condiciones, y un auto switch sin condiciones se queda apagado por diseño.

### ¿Dónde se fue la página OTA / MQTT?

Esas páginas solo aparecen en compilaciones de firmware hechas con la función
correspondiente: consulta [OTA updates](/gekko/es/guides/ota-updates/) y
[MQTT y Home Assistant](/gekko/es/guides/mqtt-home-assistant/).

## Recuperación

### Restablecimiento de fábrica

Reflashea con borrado completo (la opción de borrado del instalador web, o
`esptool erase_flash` + reflasheo). Esto borra las credenciales WiFi y todo el
registro de dispositivos: exporta antes una
[copia de seguridad](/gekko/es/guides/backup-restore/) si luego quieres
restaurar la configuración.

### ¿Qué versión de firmware estoy ejecutando?

`GET /api/system/version`, la página System en el portal, o la línea
`Gekko booting version=…` en el log serie de arranque.
