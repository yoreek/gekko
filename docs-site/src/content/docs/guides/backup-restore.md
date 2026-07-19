---
title: Backup & restore
description: Export the full Gekko device setup as one human-editable bundle and restore it anywhere.
sidebar:
  order: 5
---

Your whole device setup — every device, its configuration, the dependency
graph, display layouts, and the dashboard — exports as **one file** you can
download, keep, hand-edit, and restore on the same or another controller.

## From the portal

Open **System → Backup**:

- **Download** saves `device-setup.ndjson`.
- **Restore** uploads a bundle, with confirmation. The bundle is validated
  first; any error rejects the import **without touching the live setup**, and
  a successful restore atomically replaces all devices.

## From the command line

```sh
# backup
curl -fsS http://<device-ip>/api/device-setup/export -o device-setup.ndjson

# restore
curl -fsS -F "bundle=@device-setup.ndjson" http://<device-ip>/api/device-setup/import
```

## The bundle is plain JSON — and hand-editable

The file is NDJSON: one JSON object per line, in the same shape the REST API
accepts. You can tweak a pin number in a text editor, or even write a minimal
bundle from scratch:

```json
{"kind":"transfer_envelope","transferSchemaVersion":3}
{"kind":"device","record":{"id":4,"typeName":"gpio_switch"},"config":{"name":"Pump","enabled":true,"gpioPin":26}}
```

Only `record.id` and `record.typeName` are required per device — everything
else gets defaults. Dependency ids refer to other `record.id`s in the same
bundle, and bundles from older firmware import cleanly as long as their fields
still parse. Full format details: [`docs/backup-and-restore.md`](https://github.com/yoreek/gekko/blob/master/docs/backup-and-restore.md).

## What is (and isn't) included

**Included:** the device registry (all types, dependencies, display layouts)
and the dashboard layout.

**Not included:** WiFi credentials, MQTT settings, retained runtime state, and
firmware/OTA state.

## Automatic backups

The firmware deliberately has no built-in scheduler — the export endpoint is a
plain GET any machine on your LAN can pull. Daily cron example:

```sh
# /etc/cron.d/gekko-backup — daily at 03:00, keep 30 days
0 3 * * * user curl -fsS http://192.168.1.240/api/device-setup/export \
  -o /var/backups/gekko/device-setup-$(date +\%F).ndjson \
  && find /var/backups/gekko -name 'device-setup-*.ndjson' -mtime +30 -delete
```

Or a Home Assistant `shell_command` + time-triggered automation. See the
repository doc above for a ready-made HA snippet.

:::caution
The portal has no authentication — anyone on your network can read the bundle
(and the portal). Configs contain no WiFi/MQTT secrets, but keep the controller
on a network you trust.
:::
