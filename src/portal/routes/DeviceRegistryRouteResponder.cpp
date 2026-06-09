#include "portal/routes/DeviceRegistryRouteResponder.h"

#if defined(ARDUINO) && !defined(UNIT_TEST)
#include <ESPAsyncWebServer.h>
#endif

namespace ewfm {

#if defined(ARDUINO) && !defined(UNIT_TEST)
void DeviceRegistryRouteResponder::sendJson(AsyncWebServerRequest* request, int httpCode, JsonDocument& doc) {
    String payload;
    serializeJson(doc, payload);
    AsyncWebServerResponse* response = request->beginResponse(httpCode, "application/json", payload);
    addCorsHeaders(response);
    addNoCacheHeaders(response);
    request->send(response);
}

void DeviceRegistryRouteResponder::sendError(AsyncWebServerRequest* request, int httpCode, const char* code, const char* message) {
    DynamicJsonDocument doc(256);
    doc["success"] = false;
    doc["code"] = code;
    doc["error"] = message;
    sendJson(request, httpCode, doc);
}

void DeviceRegistryRouteResponder::sendOptions(AsyncWebServerRequest* request) {
    AsyncWebServerResponse* response = request->beginResponse(204);
    addCorsHeaders(response);
    addNoCacheHeaders(response);
    request->send(response);
}

void DeviceRegistryRouteResponder::addCorsHeaders(AsyncWebServerResponse* response) {
    if (response == nullptr) {
        return;
    }
    response->addHeader("Access-Control-Allow-Origin", "*");
    response->addHeader("Access-Control-Allow-Methods", "GET, POST, DELETE, OPTIONS");
    response->addHeader("Access-Control-Allow-Headers", "Content-Type");
    response->addHeader("Access-Control-Max-Age", "3600");
}

void DeviceRegistryRouteResponder::addNoCacheHeaders(AsyncWebServerResponse* response) {
    if (response == nullptr) {
        return;
    }
    response->addHeader("Cache-Control", "no-store, no-cache, must-revalidate, max-age=0");
    response->addHeader("Pragma", "no-cache");
    response->addHeader("Expires", "0");
}
#endif

} // namespace ewfm
