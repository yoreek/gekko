# Device Setup Backup and Restore

The device-setup transfer bundle is a **human-editable NDJSON document**: every device
config is plain JSON in the same shape the REST API accepts. You can export it from one
controller, tweak fields by hand, and upload it to another controller. The registry keeps
storing configs as versioned binary blobs in NVS; JSON exists only in the bundle and on
the wire.

- Export: `GET /api/device-setup/export` → `device-setup.ndjson`
- Import: `POST /api/device-setup/import` (multipart field `bundle`)
- Portal UI: **System → Backup** (download / restore with confirmation)

Serialization is generic over `DeviceApiAdapterRegistry::withDefaults()`
(`src/integrations/common/DeviceApiAdapter.cpp`), so every registered device type —
including future ones — round-trips without codec changes. A native test
(`test_device_setup_transfer_covers_all_registered_types`) fails if a runtime type is
registered without a REST adapter.

## Bundle format (transfer schema version 3)

One JSON object per line:

```
{"kind":"transfer_envelope","transferSchemaVersion":3,"registrySchemaVersion":1,"registryRevision":42,"deviceCount":3}
{"kind":"device","record":{"id":1001,"typeName":"i2c_bus","configVersion":1,"configRevision":5},"config":{"deps":[],"enabled":true,"name":"Main I2C","sdaPin":21,"sclPin":22,"frequencyHz":400000}}
{"kind":"device","record":{"id":1002,"typeName":"ssd1306","configVersion":4,"configRevision":2},"config":{"deps":[{"role":"i2c_bus","deviceId":1001}],"enabled":true,"name":"OLED","i2cAddress":60}}
{"kind":"layout_begin","deviceId":1002,"schemaVersion":1,"activePageId":"main","pageCount":1}
{"kind":"layout_page","deviceId":1002,"pageIndex":0,"id":"main","name":"Main","order":0,"widgetCount":1}
{"kind":"layout_widget","deviceId":1002,"pageIndex":0,"widgetIndex":0,"id":"title","type":"text","text":"Aquarium"}
{"kind":"layout_end","deviceId":1002}
{"kind":"dashboard_layout","revision":7,"layout":{"schemaVersion":1,"activePanelId":"main","panels":[...]}}
```

- `record.configVersion` documents the config struct version the exporting firmware used.
  It is informational: import always parses `config` with the **current** parser and
  re-encodes at the current version.
- Display layout records immediately follow their `device` record. `pageCount` and
  `widgetCount` bound allocation and make missing or extra records detectable. A new
  `device` record is invalid until the preceding display reaches `layout_end`.
- Every layout widget is a separate JSON line, so a bitmap never forces the parser to
  hold the complete layout JSON document in memory.
- The `dashboard_layout` line is optional and holds the free-placement dashboard.

## Hand-editing rules

- `record.id` and `record.typeName` are required per device; everything else has
  defaults. A minimal hand-written bundle is:

  ```
  {"kind":"transfer_envelope","transferSchemaVersion":3}
  {"kind":"device","record":{"id":4,"typeName":"gpio_switch"},"config":{"name":"Pump","enabled":true,"gpioPin":26}}
  ```

- `deviceCount` in the envelope is optional; when present it must match the number of
  device lines.
- `config.deps` may be omitted — the adapter derives dependencies from the config fields
  (bus device ids, layout metric sources) exactly like a REST create. When present, the
  list is parsed explicitly and the adapter adds any required layout metric dependencies
  that are missing.
- Dependency `deviceId`s refer to `record.id` values inside the same bundle; import is an
  atomic full replace of the registry, so ids are preserved as written.

## Version handling

- Import accepts transfer schema versions 1 and 2 with legacy embedded display layouts,
  plus the current ordered-record schema version 3. Export always writes version 3.
- A bundle from older firmware imports cleanly as long as its JSON fields still parse:
  missing newer fields get defaults, matching REST semantics.
- If a field was renamed/reshaped since the bundle was written and parsing fails, the
  import reports a per-device error such as
  `device 7 (ssd1306): ... (bundle config version 1 predates this firmware's version 3; update the config fields to the current format)`
  so the file can be fixed by hand. There is no automatic JSON migration layer.

## Restore behavior

1. The whole bundle is validated first; any device error rejects the import without
   touching the live registry.
2. `DeviceRegistry::restore` atomically replaces all devices (same persistence path as
   normal config edits).
3. Schema v3 display records are assembled and validated before restore. The prepared
   layout state and the optional `dashboard_layout` are applied after a successful registry
   restore. Runtime application problems are reported in the response `warnings` array.
   Dashboard widgets referencing unknown devices are pruned.

## Automatic backups (external pull)

The firmware deliberately has no built-in backup scheduler. The export endpoint is a
plain unauthenticated GET, so any external machine on the LAN can pull backups on a
schedule.

> Note: the portal has no authentication — anyone on the network can read the bundle.
> Configs contain no WiFi/MQTT secrets, but treat backups accordingly.

Cron example (daily, keep 30 days):

```sh
# /etc/cron.d/esp32-backup
0 3 * * * user curl -fsS http://192.168.1.240/api/device-setup/export \
  -o /var/backups/esp32/device-setup-$(date +\%F).ndjson \
  && find /var/backups/esp32 -name 'device-setup-*.ndjson' -mtime +30 -delete
```

Home Assistant example:

```yaml
# configuration.yaml
shell_command:
  esp32_backup: >-
    curl -fsS http://192.168.1.240/api/device-setup/export
    -o /config/backups/esp32-device-setup-{{ now().strftime('%Y-%m-%d') }}.ndjson

automation:
  - alias: Nightly ESP32 setup backup
    trigger:
      - platform: time
        at: "03:00:00"
    action:
      - service: shell_command.esp32_backup
```

Restore from a saved file:

```sh
curl -fsS -F "bundle=@device-setup-2026-07-11.ndjson" \
  http://192.168.1.240/api/device-setup/import
```

## Scope

Included: device registry (all types, deps, display layouts) and the dashboard layout.
Not included: WiFi credentials, MQTT settings, retained/persisted runtime state (kept
separate from config by design), and OTA/firmware state.
