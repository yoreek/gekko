#pragma once

#include "devices/registry/DeviceRegistry.h"
#include "integrations/common/DeviceApiAdapter.h"
#include "portal/routes/DeviceRegistryRouteParser.h"

#if defined(ARDUINO) && !defined(UNIT_TEST)
#include <ArduinoJson.h>

class AsyncWebServerRequest;
#endif

namespace ewfm {

class DeviceRegistryRouteHandlers {
public:
    DeviceRegistryRouteHandlers(DeviceRegistry& registry, const DeviceApiAdapterRegistry& adapters);

#if defined(ARDUINO) && !defined(UNIT_TEST)
    void handleList(AsyncWebServerRequest* request) const;
    void handleShow(AsyncWebServerRequest* request) const;
    void handleCreate(AsyncWebServerRequest* request, JsonVariant& json) const;
    void handleCommand(AsyncWebServerRequest* request, JsonVariant& json) const;
    void handleDelete(AsyncWebServerRequest* request) const;
    void handleFlush(AsyncWebServerRequest* request) const;
    void handleOptions(AsyncWebServerRequest* request) const;
#endif

private:
    DeviceRegistry& registry_;
    DeviceRegistryRouteParser parser_;
};

} // namespace ewfm
