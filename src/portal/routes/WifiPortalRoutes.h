#pragma once

#include "wifi/WifiDriver.h"
#include "wifi/WifiManager.h"

#if defined(ARDUINO) && !defined(UNIT_TEST)
class AsyncWebServer;
class AsyncWebServerRequest;
#endif

namespace ewfm {

class WifiPortalRoutes {
public:
    WifiPortalRoutes(WifiManager& wifiManager, IWifiDriver& wifiDriver);

#if defined(ARDUINO) && !defined(UNIT_TEST)
    void registerRoutes(AsyncWebServer& server);
#endif

private:
#if defined(ARDUINO) && !defined(UNIT_TEST)
    void handleScan(AsyncWebServerRequest* request);
    void handleStatus(AsyncWebServerRequest* request);
    void handleConfigure(AsyncWebServerRequest* request);
    void handleStartBleConfig(AsyncWebServerRequest* request);
#endif

    WifiManager& wifiManager_;
    IWifiDriver& wifiDriver_;
    bool scanStarted_{false};
};

} // namespace ewfm
