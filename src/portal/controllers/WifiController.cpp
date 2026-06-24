#include "portal/controllers/WifiController.h"

#if defined(ARDUINO) && !defined(UNIT_TEST)
#include "debug/Debug.h"
#include "portal/PortalResponses.h"

#include <ESPAsyncWebServer.h>
#include <cstdlib>
#include <cstring>
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
    server.on(
        "/api/wifi/configure", HTTP_POST, [&wifiManager, &wifiDriver](AsyncWebServerRequest* request) { (void)request; }, nullptr,
        [&wifiManager, &wifiDriver](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
            if (!BaseController::appendRequestBody(request, data, len, index, total)) {
                return;
            }

            WifiController(request, Action::Create, wifiManager, wifiDriver).dispatch(static_cast<uint8_t*>(request->_tempObject), total);
            BaseController::clearRequestBody(request);
        });
    server.on("/api/wifi/configure", HTTP_DELETE, [&wifiManager, &wifiDriver](AsyncWebServerRequest* request) {
        WifiController(request, Action::Destroy, wifiManager, wifiDriver).dispatch();
    });
    server.on(
        "/api/wifi/ble-config", HTTP_POST, [&wifiManager, &wifiDriver](AsyncWebServerRequest* request) { (void)request; }, nullptr,
        [&wifiManager, &wifiDriver](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
            if (!BaseController::appendRequestBody(request, data, len, index, total)) {
                return;
            }

            WifiController(request, Action::Cmd, wifiManager, wifiDriver).dispatch(static_cast<uint8_t*>(request->_tempObject), total);
            BaseController::clearRequestBody(request);
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
    response->print("{\"success\":true,\"status\":\"ok\",\"networks\":[");
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

    StaticJsonDocument<320> doc;
    const std::string stationIp = wifiDriver_.stationIp();
    const std::string setupApIp = wifiDriver_.setupApIp();
    doc["wifiStatus"] = statusText;
    doc["stationIp"] = stationIp;
    doc["setupApIp"] = setupApIp;
    renderOk(doc);
#endif
}

void WifiController::create() {
#if defined(ARDUINO) && !defined(UNIT_TEST)
    const JsonObjectConst input = getDoc()->as<JsonObjectConst>();
    if (!input["ssid"].is<const char*>()) {
        renderError(400, "BAD_ARGS", "ssid is required");
        return;
    }

    WiFiCredentials credentials;
    credentials.ssid = input["ssid"].as<const char*>();
    if (input["password"].is<const char*>()) {
        credentials.password = input["password"].as<const char*>();
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

void WifiController::destroy() {
#if defined(ARDUINO) && !defined(UNIT_TEST)
    const WifiManagerResult result = wifiManager_.clearCredentials();
    if (result == WifiManagerResult::Accepted) {
        StaticJsonDocument<128> doc;
        doc["status"] = "accepted";
        doc["action"] = "clear_wifi_credentials";
        sendJson(202, doc);
        return;
    }

    if (result == WifiManagerResult::StorageError) {
        renderError(500, "STORAGE_ERROR", "failed to clear wifi credentials");
        return;
    }

    renderError(409, "BUSY", "wifi manager busy");
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
