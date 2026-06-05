#pragma once

#include "provisioning/ProvisioningCoordinator.h"
#include "wifi/WifiDriver.h"

#include <memory>

namespace ewfm {

class PortalServer {
public:
    PortalServer(ProvisioningCoordinator& coordinator, IWifiDriver& wifiDriver);
    ~PortalServer();

    bool begin();
    void end();
    void tick();

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace ewfm
