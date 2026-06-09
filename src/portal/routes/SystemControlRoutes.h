#pragma once

#include "portal/routes/SystemRestartController.h"

#if defined(ARDUINO) && !defined(UNIT_TEST)
class AsyncWebServer;
class AsyncWebServerRequest;
#endif

namespace ewfm {

class DeviceRegistry;

class SystemControlRoutes {
public:
    explicit SystemControlRoutes(DeviceRegistry* registry = nullptr) : precondition_(registry) {}

    static constexpr bool restartApiEnabledForBuild() {
#if defined(WITH_SYSTEM_RESTART_API)
        return true;
#else
        return false;
#endif
    }

#if defined(ARDUINO) && !defined(UNIT_TEST)
    void registerRoutes(AsyncWebServer& server);
#endif

private:
    DeviceRegistryRestartPrecondition precondition_;

#if defined(ARDUINO) && !defined(UNIT_TEST)
    void handleRestart(AsyncWebServerRequest* request);
#endif
};

} // namespace ewfm
