#include "core/App.h"

#include "debug/Debug.h"

#if defined(ARDUINO) && !defined(UNIT_TEST)
#include <LittleFS.h>
#endif

namespace ewfm {

App::App()
    : configStore_(storage_), wifiManager_(wifiDriver_, clock_), provisioningCoordinator_(configStore_, wifiManager_),
      mobileProvisioning_(provisioningCoordinator_, clock_), portalServer_(provisioningCoordinator_, wifiDriver_) {}

bool App::begin() {
    EWFM_APP_LOG_INFO("ESP32 WiFi Manager booting");

#if defined(ARDUINO) && !defined(UNIT_TEST)
    LittleFS.begin(true);
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
    const uint32_t now = clock_.millis();
    wifiManager_.begin(config);
    mobileProvisioning_.begin(config);

    wifiManager_.tick(now);
    mobileProvisioning_.tick(now);

    if (!config.wifi.hasCredentials()) {
        startProvisioningServices(now);
    }

    return true;
}

void App::tick() {
    const uint32_t now = clock_.millis();

    wifiManager_.tick(now);
    mobileProvisioning_.tick(now);
    portalServer_.tick(now);

    if (wifiManager_.provisioningFallback() && !provisioningServicesRunning_) {
        startProvisioningServices(now);
    }
    if (wifiManager_.connected() && provisioningServicesRunning_) {
        stopProvisioningServices(now);
    }
}

void App::startProvisioningServices(uint32_t now) {
    const DeviceConfig& config = configStore_.config();
    EWFM_APP_LOG_DEBUG("starting provisioning services");
    if (config.provisioning.httpPortalEnabled) {
        portalServer_.begin();
    }
    if (config.provisioning.mobileProvisioningEnabled) {
        mobileProvisioning_.start(now);
    }
    provisioningServicesRunning_ = true;
}

void App::stopProvisioningServices(uint32_t now) {
    EWFM_APP_LOG_DEBUG("stopping provisioning services");
    portalServer_.end();
    mobileProvisioning_.stop(now);
    provisioningServicesRunning_ = false;
}

} // namespace ewfm
