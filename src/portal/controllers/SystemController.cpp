#include "portal/controllers/SystemController.h"

#include "debug/Debug.h"
#include "devices/registry/DeviceRegistry.h"

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

DeviceValidationResult flushRegistryBeforeRestart(DeviceRegistry* registry) {
    if (registry == nullptr) {
        return {};
    }
    return registry->flushNow();
}
} // namespace

void SystemController::registerRoutes(AsyncWebServer& server, DeviceRegistry* registry) {
#if defined(WITH_SYSTEM_RESTART_API)
    server.on("/api/system/restart", HTTP_POST,
              [registry](AsyncWebServerRequest* request) { SystemController(request, Action::Create, registry).dispatch(); });
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

void SystemController::create() {
#if defined(ARDUINO) && !defined(UNIT_TEST)
    const DeviceValidationResult flush = flushRegistryBeforeRestart(deviceRegistry_);
    if (!flush.ok()) {
        EWFM_PORTAL_LOG_WARN("system restart rejected: %s", flush.message);
        renderError(500, "STORAGE_ERROR", flush.message);
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
    scheduleControllerRestart();
#endif
}

} // namespace ewfm
