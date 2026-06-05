#pragma once

#include "provisioning/ProvisioningCoordinator.h"
#include "wifi/WifiDriver.h"

#if defined(ARDUINO) && !defined(UNIT_TEST)
class AsyncWebServer;
class AsyncWebServerRequest;
#endif

namespace ewfm {

class WifiPortalRoutes {
public:
    WifiPortalRoutes(ProvisioningCoordinator& coordinator, IWifiDriver& wifiDriver);

#if defined(ARDUINO) && !defined(UNIT_TEST)
    void registerRoutes(AsyncWebServer& server);
#endif

private:
#if defined(ARDUINO) && !defined(UNIT_TEST)
    void handleScan(AsyncWebServerRequest* request);
    void handleConfigure(AsyncWebServerRequest* request);
    void handleReset(AsyncWebServerRequest* request);
#endif

    ProvisioningCoordinator& coordinator_;
    IWifiDriver& wifiDriver_;
    bool scanStarted_{false};
};

} // namespace ewfm
