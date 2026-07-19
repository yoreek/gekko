#include "portal/controllers/OtaController.h"

#if defined(ARDUINO) && !defined(UNIT_TEST)
#include "portal/PortalResponses.h"

#include <ESPAsyncWebServer.h>
#include <Update.h>
#endif

namespace ewfm {

OtaController::OtaController(AsyncWebServerRequest* request, const Action action, DeviceRegistry* deviceRegistry)
    : BaseController(request, action), deviceRegistry_(deviceRegistry) {}

#if defined(ARDUINO) && !defined(UNIT_TEST)
void OtaController::registerRoutes(AsyncWebServer& server, DeviceRegistry* deviceRegistry) {
    // The status route stays registered even when web OTA is compiled out, so the UI
    // can render the disabled state instead of treating a 404 as an error.
    server.on("/api/ota/status", HTTP_GET,
              [deviceRegistry](AsyncWebServerRequest* request) { OtaController(request, Action::Show, deviceRegistry).dispatch(); });
    server.on("/api/ota/status", HTTP_OPTIONS,
              [deviceRegistry](AsyncWebServerRequest* request) { OtaController(request, Action::Options, deviceRegistry).dispatch(); });

#if defined(WITH_WEB_OTA)
    server.on(
        "/api/ota", HTTP_POST,
        [deviceRegistry](AsyncWebServerRequest* request) { OtaController(request, Action::Create, deviceRegistry).dispatch(); },
        [deviceRegistry](AsyncWebServerRequest* request, const String& filename, size_t index, uint8_t* data, size_t len, bool final) {
            OtaController(request, Action::Create, deviceRegistry).handleUpload(filename, index, data, len, final);
        });
    server.on("/api/ota", HTTP_OPTIONS,
              [deviceRegistry](AsyncWebServerRequest* request) { OtaController(request, Action::Options, deviceRegistry).dispatch(); });
#endif
}

void OtaController::handleUpload(const String& filename, size_t index, uint8_t* data, size_t len, bool final) {
    (void)filename;
    if (index == 0 && !beginUpload(request_->contentLength())) {
        return;
    }
    if (!Update.hasError() && Update.write(data, len) != len) {
        Update.abort();
        return;
    }
    if (final && !Update.hasError()) {
        Update.end(true);
    }
}

bool OtaController::beginUpload(size_t totalBytes) {
    if (totalBytes == 0 || totalBytes > ESP.getFreeSketchSpace()) {
        Update.abort();
        return false;
    }
    return Update.begin(totalBytes);
}
#endif

void OtaController::show() {
#if defined(ARDUINO) && !defined(UNIT_TEST)
    StaticJsonDocument<128> doc;
    doc["enabled"] = compiledIn();
#if defined(WITH_WEB_OTA)
    doc["freeSketchSpace"] = ESP.getFreeSketchSpace();
    doc["hasError"] = Update.hasError();
#else
    doc["freeSketchSpace"] = 0;
    doc["hasError"] = false;
#endif
    renderOk(doc);
#endif
}

void OtaController::create() {
#if defined(ARDUINO) && !defined(UNIT_TEST)
    const bool ok = !Update.hasError();
    if (ok && deviceRegistry_ != nullptr) {
        const DeviceValidationResult flush = deviceRegistry_->flushNow();
        if (!flush.ok()) {
            renderError(500, "STORAGE_ERROR", "registry flush failed before reboot");
            return;
        }
    }

    StaticJsonDocument<128> doc;
    doc["success"] = ok;
    if (ok) {
        doc["status"] = "ok";
        doc["rebooting"] = true;
    } else {
        doc["code"] = "OTA_FAILED";
        doc["error"] = "ota failed";
    }
    String payload;
    serializeJson(doc, payload);
    AsyncWebServerResponse* response = request_->beginResponse(ok ? 200 : 500, "application/json", payload);
    response->addHeader("Connection", "close");
    send(response);

    if (ok) {
        request_->client()->close();
        ESP.restart();
    }
#endif
}

} // namespace ewfm
