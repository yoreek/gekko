#pragma once

#include "config/DeviceConfig.h"
#include "portal/controllers/BaseController.h"

#if defined(ARDUINO) && !defined(UNIT_TEST)
class AsyncWebServer;
class AsyncWebServerRequest;
#endif

namespace ewfm {

class ConfigStore;
class DeviceRegistry;

// GET/PUT /api/system/persistence/settings -- debounce/max-delay for the device registry's
// deferred flash writes (see DeviceRegistry::tick()/DeviceRegistryPersistenceCoordinator). A
// successful PUT persists via ConfigStore and immediately re-applies to the live DeviceRegistry.
class PersistenceController : public BaseController {
public:
    PersistenceController(AsyncWebServerRequest* request, Action action, ConfigStore* configStore, DeviceRegistry* deviceRegistry);

#if defined(ARDUINO) && !defined(UNIT_TEST)
    static void registerRoutes(AsyncWebServer& server, ConfigStore* configStore, DeviceRegistry* deviceRegistry);
#endif

protected:
    const RulesChain* beforeChain() override;
    void index() override;  // GET /api/system/persistence/settings
    void update() override; // PUT /api/system/persistence/settings
    void options() override;

private:
    static bool parseJsonBody(BaseController& self);
    static const char* errorCodeForPersistenceError(ConfigError error);

    ConfigStore* configStore_{nullptr};
    DeviceRegistry* deviceRegistry_{nullptr};
};

} // namespace ewfm
