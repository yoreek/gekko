#include "portal/controllers/SystemController.h"

#include "debug/Debug.h"
#include "devices/registry/DeviceRegistry.h"
#include "generated/Version.h"
#include "portal/controllers/SystemRestartController.h"

#if defined(ARDUINO) && !defined(UNIT_TEST)
#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>
#include <WiFi.h>
#include <cstdlib>
#include <cstring>
#endif

namespace ewfm {

#if defined(ARDUINO) && !defined(UNIT_TEST)

void SystemController::registerRoutes(AsyncWebServer& server, DeviceRegistry* registry) {
    server.on("/api/system/version", HTTP_GET, [registry](AsyncWebServerRequest* request) {
        SystemController(request, Action::Show, registry).dispatch();
    });
    server.on("/api/system/version", HTTP_OPTIONS, [registry](AsyncWebServerRequest* request) {
        SystemController(request, Action::Options, registry).dispatch();
    });

#if defined(WITH_SYSTEM_RESTART_API)
    server.on(
        "/api/system/restart", HTTP_POST, [registry](AsyncWebServerRequest* request) { (void)request; }, nullptr,
        [registry](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
            if (!BaseController::appendRequestBody(request, data, len, index, total)) {
                return;
            }

            SystemController(request, Action::Create, registry).dispatch(static_cast<uint8_t*>(request->_tempObject), total);
            BaseController::clearRequestBody(request);
        });
    server.on("/api/system/restart", HTTP_OPTIONS,
              [registry](AsyncWebServerRequest* request) { SystemController(request, Action::Options, registry).dispatch(); });
#else
    (void)server;
    (void)registry;
#endif
}
#endif

SystemController::SystemController(AsyncWebServerRequest* request, const Action action, DeviceRegistry* registry)
    : BaseController(request, action), deviceRegistry_(registry) {}

void SystemController::show() {
#if defined(ARDUINO) && !defined(UNIT_TEST)
    StaticJsonDocument<128> doc;
    doc["version"] = EWFM_FIRMWARE_VERSION;
    doc["buildDate"] = EWFM_FIRMWARE_BUILD_DATE;
    renderOk(doc);
#endif
}

void SystemController::create() {
#if defined(ARDUINO) && !defined(UNIT_TEST)
    DeviceRegistryRestartPrecondition precondition(deviceRegistry_);
    const SystemRestartDecision decision = SystemRestartController::requestRestart(precondition);
    if (!decision.ok()) {
        EWFM_PORTAL_LOG_WARN("system restart rejected: %s", decision.validation.message);
        renderError(500, "STORAGE_ERROR", decision.validation.message);
        return;
    }

    StaticJsonDocument<128> doc;
    doc["success"] = true;
    doc["rebooting"] = true;
    String payload;
    serializeJson(doc, payload);
    AsyncWebServerResponse* response = request_->beginResponse(200, "application/json", payload);
    response->addHeader("Connection", "close");
    send(response);

    EWFM_PORTAL_LOG_INFO("system restart requested via API");
    SystemRestartController::scheduleReboot();
#endif
}

} // namespace ewfm
