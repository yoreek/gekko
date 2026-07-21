---
title: Primer arranque y configuración WiFi
description: Conecta un controlador Gekko recién flasheado a tu red WiFi a través de su punto de acceso de configuración.
sidebar:
  order: 4
---

Gekko se entrega **sin credenciales WiFi incrustadas**. En el primer arranque
el dispositivo abre su propio punto de acceso de configuración y tú configuras
tu red desde el portal.

## Conectarse mediante el punto de acceso de configuración

1. Enciende la placa recién flasheada. En pocos segundos inicia un punto de
   acceso WiFi abierto llamado **`gekko-<suffix>`**, donde el sufijo proviene
   de la dirección MAC de la placa, así que dos controladores juntos nunca
   chocan.
2. Conéctate a ese punto de acceso desde un teléfono o un portátil. En la
   mayoría de los sistemas aparecerá un aviso de portal cautivo; si no, abre
   el portal directamente por IP - `http://192.168.4.1/` (la dirección AP
   estándar del ESP32).
3. Abre la página **WiFi** en el portal. El dispositivo escanea las redes
   cercanas y muestra una lista.
4. Elige tu red, introduce la contraseña y guarda.
5. El dispositivo se conecta a tu red como estación. El AP de configuración lo
   gestiona la máquina de estados de WiFi - permanece disponible hasta que la
   conexión de estación se establece, así que un error tipográfico en la
   contraseña nunca te deja fuera.

Después de una conexión exitosa, abre el portal en la dirección que tu router
asignó al dispositivo (consulta la lista de clientes del router o la línea de
log serie del dispositivo). A partir de ahí el portal se sirve en tu red
normal.

## Si falla la conexión

Las credenciales guardadas de una red inaccesible no dejan el dispositivo
inutilizado: los reintentos de conexión de estación dependen de timeouts y el
AP de configuración más el portal permanecen disponibles todo el tiempo.
Reconéctate al AP y corrige la configuración.

## Alternativa: aprovisionamiento BLE

Las compilaciones con aprovisionamiento móvil habilitado también pueden
recibir credenciales WiFi por **Bluetooth LE** usando una app de provisioning
compatible con Espressif (Android/iOS). El modo de configuración BLE solo
arranca tras una petición explícita desde el portal o la API, corre con un
timeout de sesión y nunca altera las credenciales almacenadas salvo que la app
envíe correctamente unas nuevas. Si has flasheado la imagen por defecto, usa
el flujo del AP de configuración de arriba: siempre está disponible.

## Siguiente

Una vez el dispositivo esté en tu red, haz el
[recorrido del portal](/gekko/es/getting-started/portal-tour/) o salta
directamente a [añadir tu primer dispositivo](/gekko/es/getting-started/first-device/).
