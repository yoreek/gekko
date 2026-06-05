#include "portal/PortalAssets.h"

namespace ewfm {

const char* portalHtml() {
    return R"HTML(
<!doctype html>
<html>
<head><meta name="viewport" content="width=device-width,initial-scale=1"><title>ESP32 WiFi Setup</title></head>
<body>
<h1>ESP32 WiFi Setup</h1>
<form method="post" action="/api/wifi/configure">
<input name="ssid" placeholder="SSID" required>
<input name="password" placeholder="Password" type="password">
<button type="submit">Save</button>
</form>
</body>
</html>
)HTML";
}

} // namespace ewfm
