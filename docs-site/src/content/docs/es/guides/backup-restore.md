---
title: Copia de seguridad y restauración
description: Exporta toda la configuración del dispositivo Gekko como un único paquete editable por humanos y restáuralo donde quieras.
sidebar:
  order: 5
---

Toda la configuración de tu dispositivo - cada dispositivo, su configuración,
el grafo de dependencias, los diseños de pantalla y el panel - se exporta
como **un solo archivo** que puedes descargar, guardar, editar a mano y
restaurar en el mismo controlador o en otro distinto.

## Desde el portal

Abre **System → Backup**:

- **Download** guarda `device-setup.ndjson`.
- **Restore** sube un paquete con confirmación. El paquete se valida antes; si
  hay cualquier error, el import se rechaza **sin tocar la configuración
  activa**, y una restauración correcta reemplaza todos los dispositivos de
  forma atómica.

## Desde la línea de comandos

```sh
# backup
curl -fsS http://<device-ip>/api/device-setup/export -o device-setup.ndjson

# restore
curl -fsS -F "bundle=@device-setup.ndjson" http://<device-ip>/api/device-setup/import
```

## El paquete es JSON puro y se puede editar a mano

El archivo es NDJSON: un objeto JSON por línea, con la misma forma que acepta
la API REST. Puedes ajustar un número de pin en un editor de texto o incluso
escribir un paquete mínimo desde cero:

```json
{"kind":"transfer_envelope","transferSchemaVersion":3}
{"kind":"device","record":{"id":4,"typeName":"gpio_switch"},"config":{"name":"Pump","enabled":true,"gpioPin":26}}
```

Solo `record.id` y `record.typeName` son obligatorios por dispositivo; todo lo
demás recibe valores por defecto. Los ids de dependencias apuntan a otros
`record.id` del mismo paquete, y los paquetes de firmware antiguos importan sin
problemas mientras sus campos sigan pudiéndose analizar. Detalles completos del
formato: [`docs/backup-and-restore.md`](https://github.com/yoreek/gekko/blob/master/docs/backup-and-restore.md).

## Qué incluye y qué no

**Incluye:** el registro de dispositivos (todos los tipos, dependencias,
layouts de pantalla) y el layout del panel.

**No incluye:** credenciales WiFi, ajustes MQTT, estado retenido en runtime ni
estado de firmware/OTA.

## Copias automáticas

El firmware no lleva programador integrado a propósito - el endpoint de
exportación es un simple GET que cualquier máquina de tu LAN puede consultar.
Ejemplo de cron diario:

```sh
# /etc/cron.d/gekko-backup — diario a las 03:00, conservar 30 días
0 3 * * * user curl -fsS http://192.168.1.240/api/device-setup/export \
  -o /var/backups/gekko/device-setup-$(date +\%F).ndjson \
  && find /var/backups/gekko -name 'device-setup-*.ndjson' -mtime +30 -delete
```

O una `shell_command` de Home Assistant más una automatización basada en hora.
Consulta el documento del repositorio de arriba para un ejemplo listo para HA.

:::caution
El portal no tiene autenticación: cualquiera en tu red puede leer el paquete
(y también el portal). Las configuraciones no contienen secretos de WiFi/MQTT,
pero mantén el controlador en una red en la que confíes.
:::
