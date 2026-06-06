#include "core/App.h"

#include "debug/Debug.h"

#if defined(ARDUINO) && !defined(UNIT_TEST)
#include <LittleFS.h>
#endif

namespace ewfm {

App::App()
    : configStore_(storage_), wifiManager_(wifiDriver_), provisioningCoordinator_(configStore_, wifiManager_),
      mobileProvisioning_(provisioningCoordinator_, wifiManager_, clock_), portalServer_(provisioningCoordinator_, wifiDriver_) {}

bool App::begin() {
    EWFM_APP_LOG_INFO("ESP32 WiFi Manager booting");

#if defined(ARDUINO) && !defined(UNIT_TEST)
    LittleFS.begin(true, "/littlefs", 10, "littlefs");
#endif

    if (!configStore_.begin()) {
        EWFM_APP_LOG_INFO("ConfigStore begin failed");
        return false;
    }

    ValidationResult loaded = configStore_.load();
    if (!loaded.ok()) {
        EWFM_APP_LOG_INFO("Config load warning: %s", loaded.message);
    }

    const DeviceConfig& config = configStore_.config();
    wifiManager_.begin(config);
    mobileProvisioning_.begin(config);
    portalServer_.begin();
#if defined(WITH_ARDUINO_OTA)
    otaService_.begin(config.deviceName, wifiManager_);
#endif

    return true;
}

void App::tick() {
    const uint32_t now = clock_.millis();

    wifiManager_.tick(now);
    mobileProvisioning_.tick(now);
    portalServer_.tick(now);
#if defined(WITH_ARDUINO_OTA)
    otaService_.tick(now);
#endif
}

} // namespace ewfm
