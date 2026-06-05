#pragma once

#include "config/ConfigStore.h"
#include "wifi/WifiManager.h"

#include <atomic>

namespace ewfm {

enum class ProvisioningResult {
    Accepted,
    InvalidInput,
    StorageError,
};

class ProvisioningCoordinator {
public:
    ProvisioningCoordinator(ConfigStore& configStore, WifiManager& wifiManager);

    ProvisioningResult submitWifiCredentials(const WiFiCredentials& credentials);
    ProvisioningResult submitWifiCredentialsAt(const WiFiCredentials& credentials, uint32_t now);
    void resetWifiCredentials();
    void resetWifiCredentialsAt(uint32_t now);
    bool requestMobileProvisioningReentry();
    bool takeMobileProvisioningReentryRequest();
    bool hasWifiCredentials() const;
    bool mobileProvisioningEnabled() const;
    bool wifiProvisioningFallback() const;

private:
    ConfigStore& configStore_;
    WifiManager& wifiManager_;
    std::atomic<bool> mobileProvisioningReentryRequested_{false};
};

} // namespace ewfm
