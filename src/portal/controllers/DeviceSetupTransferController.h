#pragma once

#include "devices/registry/DeviceRegistry.h"
#include "portal/controllers/BaseController.h"

#if defined(ARDUINO) && !defined(UNIT_TEST)
class AsyncWebServer;
class AsyncWebServerRequest;
#endif

namespace ewfm {

class DeviceSetupTransferController : public BaseController {
public:
    DeviceSetupTransferController(AsyncWebServerRequest* request, Action action, DeviceRegistry& registry);

#if defined(ARDUINO) && !defined(UNIT_TEST)
    static void registerRoutes(AsyncWebServer& server, DeviceRegistry& registry);
#endif

protected:
    const RulesChain* beforeChain() override;
    void show() override;
    void create() override;
    void options() override;

private:
    DeviceRegistry& registry_;
};

} // namespace ewfm
