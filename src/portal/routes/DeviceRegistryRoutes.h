#pragma once

#include "devices/registry/DeviceRegistry.h"
#include "integrations/common/DeviceApiAdapter.h"
#include "portal/routes/DeviceRegistryRouteHandlers.h"

#if defined(ARDUINO) && !defined(UNIT_TEST)
class AsyncWebServer;
#endif

namespace ewfm {

class DeviceRegistryRoutes {
public:
    explicit DeviceRegistryRoutes(DeviceRegistry& registry);

#if defined(ARDUINO) && !defined(UNIT_TEST)
    void registerRoutes(AsyncWebServer& server);
#endif

private:
    DeviceRegistry& registry_;
    DeviceApiAdapterRegistry adapters_;
    DeviceRegistryRouteHandlers handlers_;
};

} // namespace ewfm
