#pragma once

#if defined(ARDUINO) && !defined(UNIT_TEST)
#include <ArduinoJson.h>

class AsyncWebServerRequest;
#endif

namespace ewfm {

class DeviceRegistryRouteResponder {
public:
#if defined(ARDUINO) && !defined(UNIT_TEST)
    static void sendJson(AsyncWebServerRequest* request, int httpCode, JsonDocument& doc);
    static void sendError(AsyncWebServerRequest* request, int httpCode, const char* code, const char* message);
    static void sendOptions(AsyncWebServerRequest* request);
#endif
};

} // namespace ewfm
