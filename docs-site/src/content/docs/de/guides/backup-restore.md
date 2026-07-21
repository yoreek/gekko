---
title: Backup & Restore
description: Den kompletten Gekko-Geratestand als ein menschenlesbares Bundle exportieren und ueberall wiederherstellen.
sidebar:
  order: 5
---

Dein gesamtes Geraetesetup - jedes Geraet, seine Konfiguration, der
Abhaengigkeitsgraph, Display-Layouts und das Dashboard - exportiert als **eine
Datei**, die du herunterladen, aufbewahren, von Hand bearbeiten und auf
dieselbe oder eine andere Steuerung wiederherstellen kannst.

## Aus dem Portal

Oeffne **System -> Backup**:

- **Download** speichert `device-setup.ndjson`.
- **Restore** laedt ein Bundle mit Bestaetigung hoch. Das Bundle wird zuerst
  validiert; jeder Fehler lehnt den Import ab, **ohne das laufende Setup zu
  beruehren**, und ein erfolgreicher Restore ersetzt atomar alle Geraete.

## Aus der Kommandozeile

```sh
# backup
curl -fsS http://<device-ip>/api/device-setup/export -o device-setup.ndjson

# restore
curl -fsS -F "bundle=@device-setup.ndjson" http://<device-ip>/api/device-setup/import
```

## Das Bundle ist reines JSON - und von Hand editierbar

Die Datei ist NDJSON: ein JSON-Objekt pro Zeile, im selben Format, das die
REST-API akzeptiert. Du kannst in einem Texteditor eine Pin-Nummer anpassen
oder sogar ein minimales Bundle von Grund auf schreiben:

```json
{"kind":"transfer_envelope","transferSchemaVersion":3}
{"kind":"device","record":{"id":4,"typeName":"gpio_switch"},"config":{"name":"Pump","enabled":true,"gpioPin":26}}
```

Pro Geraet sind nur `record.id` und `record.typeName` erforderlich - alles
andere bekommt Standardwerte. Dependency-Ids beziehen sich auf andere
`record.id`s im selben Bundle, und Bundles aelterer Firmware lassen sich sauber
importieren, solange ihre Felder noch geparst werden koennen. Vollstaendige
Formatdetails: [`docs/backup-and-restore.md`](https://github.com/yoreek/gekko/blob/master/docs/backup-and-restore.md).

## Was enthalten ist - und was nicht

**Enthalten:** das Geraeteregister (alle Typen, Abhaengigkeiten,
Display-Layouts) und das Dashboard-Layout.

**Nicht enthalten:** WiFi-Zugangsdaten, MQTT-Einstellungen, retained runtime
state und Firmware-/OTA-Zustand.

## Automatische Backups

Die Firmware hat bewusst keinen eingebauten Scheduler - der Export-Endpunkt
ist ein einfaches GET, das jede Maschine in deinem LAN abrufen kann. Beispiel
fuer einen taeglichen Cronjob:

```sh
# /etc/cron.d/gekko-backup — taeglich um 03:00, 30 Tage aufbewahren
0 3 * * * user curl -fsS http://192.168.1.240/api/device-setup/export \
  -o /var/backups/gekko/device-setup-$(date +\%F).ndjson \
  && find /var/backups/gekko -name 'device-setup-*.ndjson' -mtime +30 -delete
```

Oder eine Home-Assistant-`shell_command` plus zeitgesteuerte Automation. Siehe
das Repository-Dokument oben fuer ein fertiges HA-Snippet.

:::caution
Das Portal hat keine Authentifizierung - jeder in deinem Netzwerk kann das
Bundle lesen (und das Portal auch). Konfigurationen enthalten keine WiFi-/
MQTT-Geheimnisse, aber halte den Controller in einem Netz, dem du vertraust.
:::
