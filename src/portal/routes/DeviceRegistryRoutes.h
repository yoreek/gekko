#pragma once

#include "devices/api/DeviceApiAdapter.h"
#include "devices/registry/DeviceRegistry.h"

#if defined(ARDUINO) && !defined(UNIT_TEST)
#include <ArduinoJson.h>

class AsyncWebServer;
class AsyncWebServerRequest;
class AsyncWebServerResponse;
#endif

namespace ewfm {

class DeviceRegistryRoutes {
public:
    explicit DeviceRegistryRoutes(DeviceRegistry& registry);

#if defined(ARDUINO) && !defined(UNIT_TEST)
    void registerRoutes(AsyncWebServer& server);
#endif

private:
#if defined(ARDUINO) && !defined(UNIT_TEST)
    void handleList(AsyncWebServerRequest* request);
    void handleShow(AsyncWebServerRequest* request);
    void handleCreate(AsyncWebServerRequest* request, JsonVariant& json);
    void handleDelete(AsyncWebServerRequest* request);
    void handleFlush(AsyncWebServerRequest* request);
    void handleOptions(AsyncWebServerRequest* request);

    bool parseDeviceId(const AsyncWebServerRequest* request, DeviceId& deviceId) const;
    const DeviceRecord* findRecord(const AsyncWebServerRequest* request) const;
    const IDeviceApiAdapter* findAdapterForRecord(const DeviceRecord& record) const;
    const IDeviceApiAdapter* findAdapterForCreate(const JsonVariant& json, std::string& error) const;
    static void addCorsHeaders(AsyncWebServerResponse* response);
    static void addNoCacheHeaders(AsyncWebServerResponse* response);
    static void sendJson(AsyncWebServerRequest* request, int httpCode, JsonDocument& doc);
    static void sendError(AsyncWebServerRequest* request, int httpCode, const char* code, const char* message);
#endif

    DeviceRegistry& registry_;
    DeviceApiAdapterRegistry adapters_;
};

} // namespace ewfm
