#include "portal/routes/WifiPortalRoutes.h"

#if defined(ARDUINO) && !defined(UNIT_TEST)
#include "debug/Debug.h"
#include "portal/PortalResponses.h"

#include <ESPAsyncWebServer.h>
#endif

namespace ewfm {

WifiPortalRoutes::WifiPortalRoutes(WifiManager& wifiManager, IWifiDriver& wifiDriver)
    : wifiManager_(wifiManager), wifiDriver_(wifiDriver) {}

#if defined(ARDUINO) && !defined(UNIT_TEST)
void WifiPortalRoutes::registerRoutes(AsyncWebServer& server) {
    server.on("/api/wifi/scan", HTTP_GET, [this](AsyncWebServerRequest* request) { handleScan(request); });

    server.on("/api/wifi/status", HTTP_GET, [this](AsyncWebServerRequest* request) { handleStatus(request); });

    server.on("/api/wifi/configure", HTTP_POST, [this](AsyncWebServerRequest* request) { handleConfigure(request); });

    server.on("/api/wifi/ble-config", HTTP_POST, [this](AsyncWebServerRequest* request) { handleStartBleConfig(request); });
}

void WifiPortalRoutes::handleScan(AsyncWebServerRequest* request) {
    std::vector<WifiNetwork> networks;
    if (!scanStarted_) {
        scanStarted_ = wifiDriver_.startScan();
        EWFM_PORTAL_LOG_INFO("wifi scan started=%d", scanStarted_);
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

void WifiPortalRoutes::handleStatus(AsyncWebServerRequest* request) {
    const WifiDriverStatus status = wifiDriver_.status();
    const char* statusText = "idle";
    switch (status) {
    case WifiDriverStatus::Connected:
        statusText = "connected";
        break;
    case WifiDriverStatus::Connecting:
        statusText = "connecting";
        break;
    case WifiDriverStatus::Failed:
        statusText = "failed";
        break;
    case WifiDriverStatus::Disconnected:
        statusText = "disconnected";
        break;
    case WifiDriverStatus::Idle:
        break;
    }

    AsyncResponseStream* response = request->beginResponseStream("application/json");
    response->print("{");
    response->printf("\"wifi_status\":\"%s\",", statusText);
    response->printf("\"station_ip\":\"%s\",", wifiDriver_.stationIp().c_str());
    response->printf("\"setup_ap_ip\":\"%s\"", wifiDriver_.setupApIp().c_str());
    response->print("}");
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

    WifiManagerResult result = wifiManager_.submitCredentials(credentials);
    if (result == WifiManagerResult::Accepted) {
        request->send(202, "application/json", "{\"status\":\"accepted\"}");
    } else if (result == WifiManagerResult::Busy) {
        request->send(409, "application/json", "{\"error\":\"wifi manager busy\"}");
    } else {
        request->send(400, "application/json", "{\"error\":\"invalid credentials\"}");
    }
}

void WifiPortalRoutes::handleStartBleConfig(AsyncWebServerRequest* request) {
    if (!wifiManager_.requestBleConfig()) {
        request->send(409, "application/json", "{\"error\":\"ble config disabled\"}");
        return;
    }

    request->send(202, "application/json", "{\"status\":\"accepted\",\"action\":\"start_ble_config\"}");
}
#endif

} // namespace ewfm
