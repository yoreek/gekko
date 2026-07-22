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

## Build a setup bundle manually

This format is also an integration boundary: an installer, a provisioning
script, or a third-party project builder can generate a bundle without driving
the portal device by device.

Start from an export from the same firmware version whenever possible. Keep the
first bundle small, import it into a spare controller, and only then add the
rest of the device graph.

1. Write a `transfer_envelope` line first. Use `transferSchemaVersion: 3` for
   a new file.
2. Add one `device` line for every device. Give each a unique numeric
   `record.id` and the exact registered `record.typeName`.
3. Put dependency IDs in `config.deps` (or the type-specific config fields).
   Each ID must refer to another device in the same file.
4. Keep display layout records immediately after their display device, if the
   bundle contains a display. The full format reference explains these ordered
   records and the optional dashboard line.
5. Import the file. Gekko validates the **whole** bundle before replacing the
   live setup; correct any reported error in the file and import again.

Do not copy a runtime status, a Wi-Fi password, or MQTT credentials into the
bundle: they are deliberately outside this format.

## Roll out one bundle to several controllers

Once one controller has a verified setup, export it and keep the file in a
version-controlled directory such as `gekko-setups/`. Treat it as a template:
give every controller its own copy when a pin map, display address, or device
name differs.

For each target controller:

1. Check its firmware is compatible with the bundle and that the physical pins
   and connected hardware match the template.
2. Import the bundle through **System → Backup**, or upload it with `curl`.
3. Verify the dependency graph and every physical output at a safe low level.
4. Export the target immediately after acceptance and save that export as its
   controller-specific baseline.

For a small lab or installation fleet, the same command can be used for each
known address:

```sh
for host in 192.168.1.241 192.168.1.242 192.168.1.243; do
  curl -fsS -F "bundle=@light-template.ndjson" \
    "http://$host/api/device-setup/import"
done
```

Run this only after testing the file on one controller. Import is an atomic
full replacement, so the template must contain every device that should remain
on each target.

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
