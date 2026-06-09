#include "portal/PortalAssets.h"

namespace ewfm {

const char* portalHtml() {
    return R"HTML(
<!doctype html>
<html>
<head>
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>ESP32 WiFi Setup</title>
<style>
body{font-family:system-ui,sans-serif;max-width:640px;margin:24px auto;padding:0 16px;line-height:1.4}
label{display:block;margin-top:12px}
input,button{font:inherit;width:100%;box-sizing:border-box;padding:10px;margin-top:6px}
button{margin-top:16px}
#status{margin:16px 0;padding:12px;border:1px solid #ccc;border-radius:6px;background:#f8f8f8}
.muted{color:#666}
</style>
</head>
<body>
<h1>ESP32 WiFi Setup</h1>
<div id="status" class="muted">Loading status...</div>
<form method="post" action="/api/wifi/configure">
<label>SSID<input name="ssid" placeholder="SSID" required></label>
<label>Password<input name="password" placeholder="Password" type="password"></label>
<button type="submit">Save</button>
</form>
<button id="ble-config" type="button">Start BLE config mode</button>
<hr>
<h2>Dynamic Devices</h2>
<form id="create-device-form">
<label>New Dummy Name<input id="device-name" placeholder="dummy-1" required></label>
<button type="submit">Create Dummy</button>
</form>
<div style="display:grid;grid-template-columns:1fr 1fr;gap:8px;margin-top:12px">
<input id="cmd-device-id" placeholder="device_id">
<input id="cmd-payload" placeholder="payload (rename/status/custom)">
</div>
<div style="display:grid;grid-template-columns:1fr 1fr 1fr;gap:8px;margin-top:8px">
<button type="button" id="cmd-rename">Rename</button>
<button type="button" id="cmd-enable">Enable</button>
<button type="button" id="cmd-disable">Disable</button>
<button type="button" id="cmd-delete">Delete</button>
<button type="button" id="cmd-fault">Set fault</button>
<button type="button" id="cmd-ready">Set ready</button>
</div>
<pre id="devices" style="margin-top:12px;padding:12px;border:1px solid #ccc;border-radius:6px;background:#f8f8f8;overflow:auto;white-space:pre-wrap"></pre>
<script>
async function refreshStatus() {
  try {
    const response = await fetch('/api/wifi/status', {cache: 'no-store'});
    const status = await response.json();
    const lines = [];
    lines.push('WiFi: ' + (status.wifi_status || 'unknown'));
    if (status.station_ip) {
      lines.push('Station IP: ' + status.station_ip);
    }
    if (status.setup_ap_ip) {
      lines.push('Setup AP IP: ' + status.setup_ap_ip);
    }
    document.getElementById('status').textContent = lines.join(' | ');
  } catch (error) {
    document.getElementById('status').textContent = 'Status unavailable';
  }
}
async function startBleConfig() {
  const button = document.getElementById('ble-config');
  button.disabled = true;
  try {
    const response = await fetch('/api/wifi/ble-config', {method: 'POST'});
    const payload = await response.json();
    document.getElementById('status').textContent = payload.status || payload.error || 'Request sent';
  } catch (error) {
    document.getElementById('status').textContent = 'BLE config request failed';
  } finally {
    button.disabled = false;
  }
}
async function listDevices() {
  try {
    const response = await fetch('/api/devices', {cache: 'no-store'});
    const payload = await response.json();
    const lines = [];
    lines.push('registry_revision=' + (payload.registry_revision || 0) + ' pending=' + (!!payload.pending_persistence));
    for (const device of (payload.devices || [])) {
      lines.push(
        '#' + device.device_id +
        ' [' + (device.type || device.type_id || '?') + ']' +
        ' ' + (device.name || '') +
        ' enabled=' + (!!device.enabled) +
        ' lifecycle=' + (device.lifecycle_status || '?') +
        ' effective=' + (device.effective_status || '?')
      );
    }
    document.getElementById('devices').textContent = lines.join('\n');
  } catch (error) {
    document.getElementById('devices').textContent = 'Device list unavailable';
  }
}
async function createDummy(event) {
  event.preventDefault();
  const name = document.getElementById('device-name').value.trim();
  if (!name) return;
  try {
    const response = await fetch('/api/devices', {
      method: 'POST',
      headers: {'Content-Type': 'application/json'},
      body: JSON.stringify({type: 'dummy', name})
    });
    const payload = await response.json();
    document.getElementById('status').textContent = payload.success ? 'Device created' : (payload.error || 'Create failed');
  } catch (error) {
    document.getElementById('status').textContent = 'Create request failed';
  } finally {
    listDevices();
  }
}
async function sendDeviceCommand(command, payloadOverride) {
  const rawId = document.getElementById('cmd-device-id').value.trim();
  const payload = payloadOverride !== undefined ? payloadOverride : document.getElementById('cmd-payload').value;
  const deviceId = Number(rawId);
  if (!deviceId) {
    document.getElementById('status').textContent = 'device_id is required';
    return;
  }
  try {
    const response = await fetch('/api/devices/command', {
      method: 'POST',
      headers: {'Content-Type': 'application/json'},
      body: JSON.stringify({device_id: deviceId, command, payload})
    });
    const body = await response.json();
    document.getElementById('status').textContent = body.success ? 'Command accepted' : (body.error || 'Command failed');
  } catch (error) {
    document.getElementById('status').textContent = 'Command request failed';
  } finally {
    listDevices();
  }
}
document.getElementById('ble-config').addEventListener('click', startBleConfig);
document.getElementById('create-device-form').addEventListener('submit', createDummy);
document.getElementById('cmd-rename').addEventListener('click', () => sendDeviceCommand('rename'));
document.getElementById('cmd-enable').addEventListener('click', () => sendDeviceCommand('enable', ''));
document.getElementById('cmd-disable').addEventListener('click', () => sendDeviceCommand('disable', ''));
document.getElementById('cmd-delete').addEventListener('click', () => sendDeviceCommand('delete', ''));
document.getElementById('cmd-fault').addEventListener('click', () => sendDeviceCommand('set_status', 'fault'));
document.getElementById('cmd-ready').addEventListener('click', () => sendDeviceCommand('set_status', 'ready'));
refreshStatus();
listDevices();
setInterval(refreshStatus, 2000);
setInterval(listDevices, 2500);
</script>
</body>
</html>
)HTML";
}

} // namespace ewfm
