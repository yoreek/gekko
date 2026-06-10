#include "portal/controllers/WifiController.h"

#if defined(ARDUINO) && !defined(UNIT_TEST)
#include "debug/Debug.h"
#include "portal/PortalResponses.h"

#include <ESPAsyncWebServer.h>
#include <vector>
#endif

namespace ewfm {

bool WifiController::scanStarted_ = false;

WifiController::WifiController(AsyncWebServerRequest* request, const Action action, WifiManager& wifiManager, IWifiDriver& wifiDriver)
    : BaseController(request, action), wifiManager_(wifiManager), wifiDriver_(wifiDriver) {}

#if defined(ARDUINO) && !defined(UNIT_TEST)
void WifiController::registerRoutes(AsyncWebServer& server, WifiManager& wifiManager, IWifiDriver& wifiDriver) {
    server.on("/api/wifi/scan", HTTP_GET, [&wifiManager, &wifiDriver](AsyncWebServerRequest* request) {
        WifiController(request, Action::Index, wifiManager, wifiDriver).dispatch();
    });
    server.on("/api/wifi/status", HTTP_GET, [&wifiManager, &wifiDriver](AsyncWebServerRequest* request) {
        WifiController(request, Action::Show, wifiManager, wifiDriver).dispatch();
    });
    server.on("/api/wifi/configure", HTTP_POST, [&wifiManager, &wifiDriver](AsyncWebServerRequest* request) {
        WifiController(request, Action::Create, wifiManager, wifiDriver).dispatch();
    });
    server.on("/api/wifi/ble-config", HTTP_POST, [&wifiManager, &wifiDriver](AsyncWebServerRequest* request) {
        WifiController(request, Action::Cmd, wifiManager, wifiDriver).dispatch();
    });
    server.on("/api/wifi/scan", HTTP_OPTIONS, [&wifiManager, &wifiDriver](AsyncWebServerRequest* request) {
        WifiController(request, Action::Options, wifiManager, wifiDriver).dispatch();
    });
    server.on("/api/wifi/status", HTTP_OPTIONS, [&wifiManager, &wifiDriver](AsyncWebServerRequest* request) {
        WifiController(request, Action::Options, wifiManager, wifiDriver).dispatch();
    });
    server.on("/api/wifi/configure", HTTP_OPTIONS, [&wifiManager, &wifiDriver](AsyncWebServerRequest* request) {
        WifiController(request, Action::Options, wifiManager, wifiDriver).dispatch();
    });
    server.on("/api/wifi/ble-config", HTTP_OPTIONS, [&wifiManager, &wifiDriver](AsyncWebServerRequest* request) {
        WifiController(request, Action::Options, wifiManager, wifiDriver).dispatch();
    });
}
#endif

void WifiController::index() {
#if defined(ARDUINO) && !defined(UNIT_TEST)
    std::vector<WifiNetwork> networks;
    if (!scanStarted_) {
        scanStarted_ = wifiDriver_.startScan();
        EWFM_PORTAL_LOG_INFO("wifi scan started=%d", scanStarted_);
        StaticJsonDocument<128> doc;
        doc["status"] = "scanning";
        sendJson(202, doc);
        return;
    }
    if (!wifiDriver_.scanComplete(networks, 20)) {
        StaticJsonDocument<128> doc;
        doc["status"] = "scanning";
        sendJson(202, doc);
        return;
    }

    scanStarted_ = false;
    AsyncResponseStream* response = request_->beginResponseStream("application/json");
    response->print("{\"status\":\"ok\",\"networks\":[");
    bool first = true;
    for (const auto& network : networks) {
        if (!first) {
            response->print(',');
        }
        first = false;
        StaticJsonDocument<256> item;
        JsonObject obj = item.to<JsonObject>();
        obj["ssid"] = network.ssid;
        obj["rssi"] = network.rssi;
        obj["channel"] = network.channel;
        serializeJson(item, *response);
    }
    response->print("]}");
    send(response);
#endif
}

void WifiController::show() {
#if defined(ARDUINO) && !defined(UNIT_TEST)
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

    StaticJsonDocument<256> doc;
    doc["wifi_status"] = statusText;
    doc["station_ip"] = wifiDriver_.stationIp().c_str();
    doc["setup_ap_ip"] = wifiDriver_.setupApIp().c_str();
    renderOk(doc);
#endif
}

void WifiController::create() {
#if defined(ARDUINO) && !defined(UNIT_TEST)
    if (request_ == nullptr || !request_->hasParam("ssid", true)) {
        renderError(400, "BAD_ARGS", "ssid is required");
        return;
    }

    WiFiCredentials credentials;
    credentials.ssid = request_->getParam("ssid", true)->value().c_str();
    if (request_->hasParam("password", true)) {
        credentials.password = request_->getParam("password", true)->value().c_str();
    }

    const WifiManagerResult result = wifiManager_.submitCredentials(credentials);
    if (result == WifiManagerResult::Accepted) {
        StaticJsonDocument<128> doc;
        doc["status"] = "accepted";
        sendJson(202, doc);
    } else if (result == WifiManagerResult::Busy) {
        renderError(409, "BUSY", "wifi manager busy");
    } else {
        renderError(400, "BAD_ARGS", "invalid credentials");
    }
#endif
}

void WifiController::cmd() {
#if defined(ARDUINO) && !defined(UNIT_TEST)
    if (!wifiManager_.requestBleConfig()) {
        renderError(409, "BUSY", "ble config disabled");
        return;
    }

    StaticJsonDocument<128> doc;
    doc["status"] = "accepted";
    doc["action"] = "start_ble_config";
    sendJson(202, doc);
#endif
}

} // namespace ewfm
