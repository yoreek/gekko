#include "portal/controllers/PinOccupancyController.h"

#include "devices/core/DeviceTypes.h"
#include "devices/registry/DeviceRegistry.h"

#if defined(ARDUINO) && !defined(UNIT_TEST)
#include <ESPAsyncWebServer.h>
#endif

namespace ewfm {

PinOccupancyController::PinOccupancyController(AsyncWebServerRequest* request, const Action action, DeviceRegistry* deviceRegistry)
    : BaseController(request, action), deviceRegistry_(deviceRegistry) {}

#if defined(ARDUINO) && !defined(UNIT_TEST)
void PinOccupancyController::registerRoutes(AsyncWebServer& server, DeviceRegistry* deviceRegistry) {
    server.on("/api/system/pins", HTTP_GET, [deviceRegistry](AsyncWebServerRequest* request) {
        PinOccupancyController(request, Action::Index, deviceRegistry).dispatch();
    });
    server.on("/api/system/pins", HTTP_OPTIONS, [deviceRegistry](AsyncWebServerRequest* request) {
        PinOccupancyController(request, Action::Options, deviceRegistry).dispatch();
    });
}
#endif

void PinOccupancyController::index() {
#if defined(ARDUINO) && !defined(UNIT_TEST)
    if (deviceRegistry_ == nullptr) {
        renderError(500, "INTERNAL", "device registry not available");
        return;
    }

    StaticJsonDocument<512> doc;
    JsonArray pins = doc.createNestedArray("pins");
    const DeviceId* pinOwners = deviceRegistry_->pinOwners();
    for (size_t gpio = 0; gpio < kGpioPinTableSize; ++gpio) {
        if (pinOwners[gpio] != 0U) {
            JsonObject entry = pins.createNestedObject();
            entry["gpio"] = static_cast<uint8_t>(gpio);
            entry["deviceId"] = pinOwners[gpio];
        }
    }
    renderOk(doc);
#endif
}

void PinOccupancyController::options() {
    BaseController::options();
}

} // namespace ewfm
