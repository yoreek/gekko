#include "portal/routes/DeviceRegistryRoutes.h"

#if defined(ARDUINO) && !defined(UNIT_TEST)
#include <ESPAsyncWebServer.h>
#endif

namespace ewfm {

DeviceRegistryRoutes::DeviceRegistryRoutes(DeviceRegistry& registry)
    : registry_(registry), adapters_(DeviceApiAdapterRegistry::withDefaults()), handlers_(registry_, adapters_) {}

#if defined(ARDUINO) && !defined(UNIT_TEST)
void DeviceRegistryRoutes::registerRoutes(AsyncWebServer& server) {
    server.on(AsyncURIMatcher::exact("/api/devices"), HTTP_GET, [this](AsyncWebServerRequest* request) { handlers_.handleList(request); });
    server.on(AsyncURIMatcher::exact("/api/devices"), HTTP_POST,
              [this](AsyncWebServerRequest* request, JsonVariant& json) { handlers_.handleCreate(request, json); });
    server.on(AsyncURIMatcher::exact("/api/devices"), HTTP_OPTIONS,
              [this](AsyncWebServerRequest* request) { handlers_.handleOptions(request); });
    server.on(AsyncURIMatcher::exact("/api/devices/flush"), HTTP_POST,
              [this](AsyncWebServerRequest* request) { handlers_.handleFlush(request); });
    server.on(AsyncURIMatcher::prefix("/api/devices/"), HTTP_GET,
              [this](AsyncWebServerRequest* request) { handlers_.handleShow(request); });
    server.on(AsyncURIMatcher::exact("/api/devices/command"), HTTP_POST,
              [this](AsyncWebServerRequest* request, JsonVariant& json) { handlers_.handleCommand(request, json); });
    server.on(AsyncURIMatcher::prefix("/api/devices/"), HTTP_DELETE,
              [this](AsyncWebServerRequest* request) { handlers_.handleDelete(request); });
    server.on(AsyncURIMatcher::prefix("/api/devices/"), HTTP_OPTIONS,
              [this](AsyncWebServerRequest* request) { handlers_.handleOptions(request); });
}
#endif

} // namespace ewfm
