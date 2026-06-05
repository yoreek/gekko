#include "portal/routes/WifiPortalRoutes.h"

#if defined(ARDUINO) && !defined(UNIT_TEST)
#include "debug/Debug.h"
#include "portal/PortalResponses.h"

#include <ESPAsyncWebServer.h>
#endif

namespace ewfm {

WifiPortalRoutes::WifiPortalRoutes(ProvisioningCoordinator& coordinator, IWifiDriver& wifiDriver)
    : coordinator_(coordinator), wifiDriver_(wifiDriver) {}

#if defined(ARDUINO) && !defined(UNIT_TEST)
void WifiPortalRoutes::registerRoutes(AsyncWebServer& server) {
    server.on("/api/wifi/scan", HTTP_GET, [this](AsyncWebServerRequest* request) { handleScan(request); });

    server.on("/api/wifi/configure", HTTP_POST, [this](AsyncWebServerRequest* request) { handleConfigure(request); });

    server.on("/api/wifi/reset", HTTP_POST, [this](AsyncWebServerRequest* request) { handleReset(request); });
}

void WifiPortalRoutes::handleScan(AsyncWebServerRequest* request) {
    std::vector<WifiNetwork> networks;
    if (!scanStarted_) {
        scanStarted_ = wifiDriver_.startScan();
        EWFM_PORTAL_LOG_DEBUG("wifi scan started=%d", scanStarted_);
        request->send(202, "application/json", "{\"status\":\"scanning\"}");
        return;
    }
    if (!wifiDriver_.scanComplete(networks, 20)) {
        request->send(202, "application/json", "{\"status\":\"scanning\"}");
        return;
    }

    scanStarted_ = false;
    AsyncResponseStream* response = request->beginResponseStream("application/json");
    writeWifiScanResponseJson(*response, networks);
    request->send(response);
}

void WifiPortalRoutes::handleConfigure(AsyncWebServerRequest* request) {
    if (!request->hasParam("ssid", true)) {
        request->send(400, "application/json", "{\"error\":\"ssid is required\"}");
        return;
    }

    WiFiCredentials credentials;
    credentials.ssid = request->getParam("ssid", true)->value().c_str();
    if (request->hasParam("password", true)) {
        credentials.password = request->getParam("password", true)->value().c_str();
    }

    ProvisioningResult result = coordinator_.submitWifiCredentials(credentials);
    if (result == ProvisioningResult::Accepted) {
        request->send(202, "application/json", "{\"status\":\"accepted\"}");
    } else {
        request->send(400, "application/json", "{\"error\":\"invalid credentials\"}");
    }
}

void WifiPortalRoutes::handleReset(AsyncWebServerRequest* request) {
    coordinator_.resetWifiCredentials();
    request->send(200, "application/json", "{\"status\":\"reset\"}");
}
#endif

} // namespace ewfm
