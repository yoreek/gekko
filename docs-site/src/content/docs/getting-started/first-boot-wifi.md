---
title: First boot & WiFi setup
description: Connect a freshly flashed Gekko controller to your WiFi network through its setup access point.
sidebar:
  order: 4
---

Gekko ships with **no hardcoded WiFi credentials**. On first boot the device
opens its own setup access point, and you configure your network from the
portal.

## Connect through the setup access point

1. Power the freshly flashed board. Within a few seconds it starts an open WiFi
   access point named **`gekko-<suffix>`**, where the suffix comes from the
   board's MAC address — so two controllers next to each other never collide.
2. Connect to that access point from a phone or laptop. On most systems a
   captive-portal prompt appears; if it does not, open the portal directly by
   IP — `http://192.168.4.1/` (the default ESP32 AP address).
3. Open the **WiFi** page in the portal. The device scans for nearby networks
   and shows a list.
4. Pick your network, enter the password, and save.
5. The device connects to your network as a station. The setup AP is managed by
   the WiFi state machine — it stays available until the station connection is
   established, so a typo in the password never locks you out.

After a successful connection, open the portal at the address your router
assigned to the device (check your router's client list, or the device's serial
log line). From now on the portal is served on your normal network.

## If the connection fails

Saved credentials for an unreachable network do **not** brick the device:
station connection retries are timeout-driven, and the setup AP plus the portal
remain available the whole time — reconnect to the AP and fix the settings.

## Alternative: BLE provisioning

The **Standard** firmware can also receive WiFi credentials over **Bluetooth
LE** using an Espressif-compatible provisioning app (Android/iOS). Connect a
normally open button between GPIO 32 and GND, then hold it for 3 seconds to
start BLE config mode. You can also start the mode from the WiFi page in the
portal or through the API. The session has a timeout and stored credentials
change only after the app successfully submits new ones.

The **Without BLE** firmware has no BLE provisioning code and does not reserve
GPIO 32. The setup access point and web portal described above remain available
in both firmware variants.

## Next

With the device on your network, take the
[portal tour](/gekko/getting-started/portal-tour/) or jump straight to
[adding your first device](/gekko/getting-started/first-device/).
