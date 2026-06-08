#pragma once

#include "config/ConfigStore.h"
#include "platform/ArduinoClock.h"
#include "platform/ArduinoOtaService.h"
#include "platform/ArduinoWifiDriver.h"
#include "platform/PreferencesConfigStorage.h"
#include "portal/PortalServer.h"
#include "wifi/WifiManager.h"

#include <memory>

namespace ewfm {

class App {
public:
    App();
    bool begin();
    void tick();

private:
    ArduinoClock clock_;
    PreferencesConfigStorage storage_;
    ArduinoWifiDriver wifiDriver_;
    ConfigStore configStore_;
    WifiManager wifiManager_;
    PortalServer portalServer_;
#if defined(WITH_ARDUINO_OTA)
    ArduinoOtaService otaService_;
#endif
};

} // namespace ewfm
