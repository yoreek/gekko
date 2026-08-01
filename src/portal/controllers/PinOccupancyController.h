#pragma once

#include "portal/controllers/BaseController.h"

#if defined(ARDUINO) && !defined(UNIT_TEST)
class AsyncWebServer;
class AsyncWebServerRequest;
#endif

namespace ewfm {

class DeviceRegistry;

// GET /api/system/pins -- which GPIO is currently claimed by which device, per
// docs/gpio-pin-occupancy.md. Read-only: the table itself is only ever mutated by DeviceRegistry
// as devices are created/reconfigured/removed.
class PinOccupancyController : public BaseController {
public:
    PinOccupancyController(AsyncWebServerRequest* request, Action action, DeviceRegistry* deviceRegistry);

#if defined(ARDUINO) && !defined(UNIT_TEST)
    static void registerRoutes(AsyncWebServer& server, DeviceRegistry* deviceRegistry);
#endif

protected:
    void index() override; // GET /api/system/pins
    void options() override;

private:
    DeviceRegistry* deviceRegistry_{nullptr};
};

} // namespace ewfm
