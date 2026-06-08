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
document.getElementById('ble-config').addEventListener('click', startBleConfig);
refreshStatus();
setInterval(refreshStatus, 2000);
</script>
</body>
</html>
)HTML";
}

} // namespace ewfm
