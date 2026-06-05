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
    const uint32_t now = clock_.millis();
    wifiManager_.begin(config);
    mobileProvisioning_.begin(config);

    wifiManager_.tick(now);
    mobileProvisioning_.tick(now);
    portalServer_.begin();

    if (!config.wifi.hasCredentials()) {
        startMobileProvisioning(now);
    }

    return true;
}

void App::tick() {
    const uint32_t now = clock_.millis();

    wifiManager_.tick(now);
    mobileProvisioning_.tick(now);
    portalServer_.tick(now);
    tickDevOta();

    if (provisioningCoordinator_.takeMobileProvisioningReentryRequest()) {
        restartMobileProvisioningBle(now);
        return;
    }

    const DeviceConfig& config = configStore_.config();
    const bool autoStartEligible = mobileProvisioning_.idle() || mobileProvisioning_.succeeded() || mobileProvisioning_.timedOut();
    if (!mobileProvisioning_.running()) {
        if (!config.wifi.hasCredentials() && mobileProvisioning_.idle()) {
            startMobileProvisioning(now);
            return;
        }
        if (wifiManager_.provisioningFallback() && autoStartEligible) {
            startMobileProvisioning(now);
        }
    }
}

void App::startMobileProvisioning(uint32_t now) {
    const DeviceConfig& config = configStore_.config();
    EWFM_APP_LOG_DEBUG("starting mobile provisioning");
    if (config.provisioning.mobileProvisioningEnabled) {
        mobileProvisioning_.start(now);
    }
}

void App::restartMobileProvisioningBle(uint32_t now) {
    EWFM_APP_LOG_INFO("PROV_REENTER handling");
    EWFM_APP_LOG_INFO("PROV_REENTER clearing wifi credentials and switching to provisioning fallback");
    provisioningCoordinator_.resetWifiCredentials();
    mobileProvisioning_.restartBle(now);
}

void App::tickDevOta() {
#if defined(WITH_ARDUINO_OTA)
    if (!otaService_.started() && wifiManager_.connected()) {
        otaService_.begin(configStore_.config().deviceName);
    }
    otaService_.tick();
#endif
}

} // namespace ewfm
