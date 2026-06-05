#include "provisioning/ProvisioningCoordinator.h"

#include "debug/Debug.h"

namespace ewfm {

ProvisioningCoordinator::ProvisioningCoordinator(ConfigStore& configStore, WifiManager& wifiManager)
    : configStore_(configStore), wifiManager_(wifiManager) {}

ProvisioningResult ProvisioningCoordinator::submitWifiCredentials(const WiFiCredentials& credentials) {
    ValidationResult result = configStore_.saveWifiCredentials(credentials);
    if (!result.ok()) {
        EWFM_PROV_LOG_WARN("credentials rejected: %s", result.message);
        return result.error == ConfigError::StorageError ? ProvisioningResult::StorageError : ProvisioningResult::InvalidInput;
    }
    EWFM_PROV_LOG_INFO("credentials accepted, connecting");
    wifiManager_.connect(credentials);
    return ProvisioningResult::Accepted;
}

ProvisioningResult ProvisioningCoordinator::submitWifiCredentialsAt(const WiFiCredentials& credentials, uint32_t now) {
    ValidationResult result = configStore_.saveWifiCredentials(credentials);
    if (!result.ok()) {
        EWFM_PROV_LOG_WARN("credentials rejected: %s", result.message);
        return result.error == ConfigError::StorageError ? ProvisioningResult::StorageError : ProvisioningResult::InvalidInput;
    }
    EWFM_PROV_LOG_INFO("credentials accepted, connecting");
    wifiManager_.connectAt(credentials, now);
    return ProvisioningResult::Accepted;
}

void ProvisioningCoordinator::resetWifiCredentials() {
    configStore_.clearWifiCredentials();
    EWFM_PROV_LOG_INFO("wifi credentials reset");
    wifiManager_.clearCredentials();
}

void ProvisioningCoordinator::resetWifiCredentialsAt(uint32_t now) {
    configStore_.clearWifiCredentials();
    EWFM_PROV_LOG_INFO("wifi credentials reset");
    wifiManager_.clearCredentialsAt(now);
}

bool ProvisioningCoordinator::requestMobileProvisioningReentry() {
    if (!mobileProvisioningEnabled()) {
        EWFM_PROV_LOG_WARN("mobile provisioning re-entry rejected: disabled");
        return false;
    }

    mobileProvisioningReentryRequested_.store(true);
    EWFM_APP_LOG_INFO("PROV_REENTER requested");
    EWFM_PROV_LOG_INFO("mobile provisioning re-entry requested");
    return true;
}

bool ProvisioningCoordinator::takeMobileProvisioningReentryRequest() {
    return mobileProvisioningReentryRequested_.exchange(false);
}

bool ProvisioningCoordinator::hasWifiCredentials() const {
    return configStore_.config().wifi.hasCredentials();
}

bool ProvisioningCoordinator::mobileProvisioningEnabled() const {
    return configStore_.config().provisioning.mobileProvisioningEnabled;
}

bool ProvisioningCoordinator::wifiProvisioningFallback() const {
    return wifiManager_.provisioningFallback();
}

} // namespace ewfm
