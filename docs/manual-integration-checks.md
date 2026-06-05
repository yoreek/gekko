# Manual Integration Checks

These checks require a real ESP32 device and are intentionally not part of the host Unity test baseline.

## First Boot and HTTP Provisioning

- Flash the base `esp32dev` environment over serial.
- Erase NVS or use the WiFi reset action.
- Confirm the device starts a setup AP with the device-specific SSID suffix.
- Connect a phone or laptop to the setup AP and open the portal by direct IP.
- Confirm `/api/wifi/scan` returns a bounded list of nearby networks.
- Submit valid credentials and confirm the setup AP stops after station connection.
- Submit invalid or oversized values and confirm the last valid configuration remains unchanged.

## Failed Connection Fallback

- Save credentials for an unavailable network.
- Reboot the device.
- Confirm station connection retries are timeout-driven.
- Confirm the device enters provisioning fallback without deleting saved credentials.

## Mobile Provisioning

- Enable mobile provisioning in the build/configuration.
- Start from an unprovisioned device.
- Use an Espressif-compatible Android or iOS provisioning app.
- Record transport, security mode, phone OS, app version, discovery result, credential submission result, and memory/build-size impact.
- Confirm HTTP portal provisioning remains available when mobile provisioning is disabled.

## Captive Portal

- Test direct IP access from Android, iOS, Windows, and macOS when available.
- Test common captive portal detection flows and record platforms where automatic captive popup does not appear.

## OTA

- Connect the device to WiFi and verify PlatformIO `esp32dev_ota` upload.
- Enable Web OTA only in a guarded development/admin build.
- Upload a valid firmware image and confirm reboot after successful finalization.
- Upload an oversized image and confirm the current firmware keeps running.
- Interrupt an upload and confirm the current firmware keeps running.

## Non-Blocking Runtime

- During WiFi retries, scans, provisioning sessions, portal requests, and Web OTA upload, confirm serial logs continue and the cooperative loop remains responsive.
