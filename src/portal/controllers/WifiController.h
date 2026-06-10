#pragma once

#include "portal/controllers/BaseController.h"
#include "wifi/WifiDriver.h"
#include "wifi/WifiManager.h"

#if defined(ARDUINO) && !defined(UNIT_TEST)
class AsyncWebServer;
class AsyncWebServerRequest;
#endif

namespace ewfm {

class WifiController : public BaseController {
public:
    WifiController(AsyncWebServerRequest* request, Action action, WifiManager& wifiManager, IWifiDriver& wifiDriver);

#if defined(ARDUINO) && !defined(UNIT_TEST)
    static void registerRoutes(AsyncWebServer& server, WifiManager& wifiManager, IWifiDriver& wifiDriver);
#endif

protected:
    void index() override;
    void show() override;
    void create() override;
    void cmd() override;

private:
    WifiManager& wifiManager_;
    IWifiDriver& wifiDriver_;
    static bool scanStarted_;
};

} // namespace ewfm
