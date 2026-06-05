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

void ProvisioningCoordinator::resetWifiCredentials() {
    configStore_.clearWifiCredentials();
    EWFM_PROV_LOG_INFO("wifi credentials reset");
    wifiManager_.clearCredentials();
}

} // namespace ewfm
