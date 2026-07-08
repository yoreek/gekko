#pragma once

#include "portal/controllers/BaseController.h"

#if defined(ARDUINO) && !defined(UNIT_TEST)
class AsyncWebServer;
class AsyncWebServerRequest;
#endif

namespace ewfm {

class DeviceRegistry;

class SystemController : public BaseController {
public:
    explicit SystemController(AsyncWebServerRequest* request, Action action, DeviceRegistry* registry = nullptr);

    static constexpr bool restartApiEnabledForBuild() {
#if defined(WITH_SYSTEM_RESTART_API)
        return true;
#else
        return false;
#endif
    }

#if defined(ARDUINO) && !defined(UNIT_TEST)
    static void registerRoutes(AsyncWebServer& server, DeviceRegistry* registry = nullptr);
#endif

protected:
    void create() override;
    void show() override;

private:
    DeviceRegistry* deviceRegistry_{nullptr};
};

} // namespace ewfm
