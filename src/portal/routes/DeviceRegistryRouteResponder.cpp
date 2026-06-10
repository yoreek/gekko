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
    request->send(response);
}
#endif

} // namespace ewfm
