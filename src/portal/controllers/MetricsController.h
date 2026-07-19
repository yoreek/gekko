#pragma once

#include "devices/registry/DeviceRegistry.h"
#include "portal/controllers/BaseController.h"
#include "wifi/WifiDriver.h"

#if defined(ARDUINO) && !defined(UNIT_TEST)
class AsyncWebServer;
class AsyncWebServerRequest;
#endif

namespace ewfm {

class MetricsController : public BaseController {
public:
    MetricsController(AsyncWebServerRequest* request, Action action, DeviceRegistry* registry, IWifiDriver& wifiDriver);

#if defined(ARDUINO) && !defined(UNIT_TEST)
    static void registerRoutes(AsyncWebServer& server, DeviceRegistry* registry, IWifiDriver& wifiDriver);
#endif

protected:
    void index() override;
    void show() override;
    void options() override;

private:
#if defined(ARDUINO) && !defined(UNIT_TEST)
    void streamMetrics(bool values);
#endif

    DeviceRegistry* registry_;
    IWifiDriver& wifiDriver_;
};

} // namespace ewfm
