#include "portal/routes/OtaPortalRoutes.h"

#include "devices/registry/DeviceRegistry.h"

#if defined(ARDUINO) && !defined(UNIT_TEST)
#include "portal/PortalResponses.h"

#include <ESPAsyncWebServer.h>
#include <Update.h>
#endif

namespace ewfm {

#if defined(ARDUINO) && !defined(UNIT_TEST)
void OtaPortalRoutes::registerRoutes(AsyncWebServer& server) {
#if defined(WITH_WEB_OTA)
    server.on("/api/ota/status", HTTP_GET, [this](AsyncWebServerRequest* request) { handleStatus(request); });

    server.on(
        "/api/ota", HTTP_POST, [this](AsyncWebServerRequest* request) { handleFinished(request); },
        [this](AsyncWebServerRequest* request, const String& filename, size_t index, uint8_t* data, size_t len, bool final) {
            handleUpload(request, filename, index, data, len, final);
        });
#else
    (void)server;
#endif
}

void OtaPortalRoutes::handleStatus(AsyncWebServerRequest* request) {
    AsyncResponseStream* response = request->beginResponseStream("application/json");
    writeOtaStatusResponseJson(*response, ESP.getFreeSketchSpace(), Update.hasError());
    request->send(response);
}

void OtaPortalRoutes::handleFinished(AsyncWebServerRequest* request) {
    const bool ok = !Update.hasError();
    if (ok && deviceRegistry_ != nullptr) {
        const DeviceValidationResult flush = deviceRegistry_->flushNow();
        if (!flush.ok()) {
            request->send(500, "application/json", "{\"error\":\"registry flush failed before reboot\"}");
            return;
        }
    }

    AsyncWebServerResponse* response = request->beginResponse(ok ? 200 : 500, "application/json",
                                                              ok ? "{\"status\":\"ok\",\"rebooting\":true}" : "{\"error\":\"ota failed\"}");
    response->addHeader("Connection", "close");
    request->send(response);

    if (ok) {
        request->client()->close();
        ESP.restart();
    }
}

void OtaPortalRoutes::handleUpload(AsyncWebServerRequest* request, const String& filename, size_t index, uint8_t* data, size_t len,
                                   bool final) {
    (void)filename;
    if (index == 0 && !beginUpload(request->contentLength())) {
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

bool OtaPortalRoutes::beginUpload(size_t totalBytes) {
    if (totalBytes == 0 || totalBytes > ESP.getFreeSketchSpace()) {
        Update.abort();
        return false;
    }
    return Update.begin(totalBytes);
}
#endif

} // namespace ewfm
