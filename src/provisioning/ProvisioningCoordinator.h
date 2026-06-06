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
    void resetWifiCredentials();
    bool requestMobileProvisioningReentry();
    bool takeMobileProvisioningReentryRequest();
    bool hasWifiCredentials() const;
    bool mobileProvisioningEnabled() const;
    bool wifiApMode() const;

private:
    ConfigStore& configStore_;
    WifiManager& wifiManager_;
    std::atomic<bool> mobileProvisioningReentryRequested_{false};
};

} // namespace ewfm
