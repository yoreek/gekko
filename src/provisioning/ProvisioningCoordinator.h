#pragma once

#include "config/ConfigStore.h"
#include "wifi/WifiManager.h"

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

private:
    ConfigStore& configStore_;
    WifiManager& wifiManager_;
};

} // namespace ewfm
