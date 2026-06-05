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
    wifiManager_.begin(config);
    mobileProvisioning_.begin(config);

    if (!config.wifi.hasCredentials()) {
        startProvisioningServices();
    }

    return true;
}

void App::tick() {
    wifiManager_.tick();
    mobileProvisioning_.tick();
    portalServer_.tick();

    if (wifiManager_.state() == WifiManagerState::ProvisioningFallback && !provisioningServicesRunning_) {
        startProvisioningServices();
    }
    if (wifiManager_.connected() && provisioningServicesRunning_) {
        stopProvisioningServices();
    }
}

void App::startProvisioningServices() {
    const DeviceConfig& config = configStore_.config();
    EWFM_APP_LOG_DEBUG("starting provisioning services");
    if (config.provisioning.httpPortalEnabled) {
        portalServer_.begin();
    }
    if (config.provisioning.mobileProvisioningEnabled) {
        mobileProvisioning_.start();
    }
    provisioningServicesRunning_ = true;
}

void App::stopProvisioningServices() {
    EWFM_APP_LOG_DEBUG("stopping provisioning services");
    portalServer_.end();
    mobileProvisioning_.stop();
    provisioningServicesRunning_ = false;
}

} // namespace ewfm
