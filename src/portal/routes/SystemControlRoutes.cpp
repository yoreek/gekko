#include "portal/routes/SystemControlRoutes.h"

#include "debug/Debug.h"
#include "portal/routes/DeviceRegistryRouteResponder.h"

#if defined(ARDUINO) && !defined(UNIT_TEST)
#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>
#include <WiFi.h>
#include <esp_timer.h>
#endif

namespace ewfm {

#if defined(ARDUINO) && !defined(UNIT_TEST)
namespace {
void restartTimerCallback(void*) {
    ESP.restart();
}

void scheduleControllerRestart() {
    static esp_timer_handle_t timerHandle = nullptr;
    if (timerHandle == nullptr) {
        const esp_timer_create_args_t args = {
            .callback = &restartTimerCallback,
            .arg = nullptr,
            .dispatch_method = ESP_TIMER_TASK,
            .name = "api-restart",
            .skip_unhandled_events = false,
        };
        (void)esp_timer_create(&args, &timerHandle);
    }
    if (timerHandle != nullptr) {
        (void)esp_timer_stop(timerHandle);
        (void)esp_timer_start_once(timerHandle, 200000);
    }
}
} // namespace

void SystemControlRoutes::registerRoutes(AsyncWebServer& server) {
#if defined(WITH_SYSTEM_RESTART_API)
    server.on("/api/system/restart", HTTP_POST, [this](AsyncWebServerRequest* request) { handleRestart(request); });
    server.on("/api/system/restart", HTTP_OPTIONS,
              [](AsyncWebServerRequest* request) { DeviceRegistryRouteResponder::sendOptions(request); });
#else
    (void)server;
#endif
}

void SystemControlRoutes::handleRestart(AsyncWebServerRequest* request) {
    const SystemRestartDecision decision = SystemRestartController::requestRestart(precondition_);
    if (!decision.ok()) {
        EWFM_PORTAL_LOG_WARN("system restart rejected: %s", decision.validation.message);
        DeviceRegistryRouteResponder::sendError(request, 500, "STORAGE_ERROR", decision.validation.message);
        return;
    }

    DynamicJsonDocument doc(128);
    doc["success"] = true;
    doc["rebooting"] = true;
    String payload;
    serializeJson(doc, payload);

    AsyncWebServerResponse* response = request->beginResponse(200, "application/json", payload);
    response->addHeader("Connection", "close");
    request->send(response);

    EWFM_PORTAL_LOG_INFO("system restart requested via API");
    scheduleControllerRestart();
}
#endif

} // namespace ewfm
