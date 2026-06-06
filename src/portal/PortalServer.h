#pragma once

#include "provisioning/ProvisioningCoordinator.h"
#include "wifi/WifiDriver.h"

#include <cstdint>
#include <memory>

namespace ewfm {

enum class PortalRuntimeState {
    Idle,
    WaitingForNetwork,
    Starting,
    Running,
    Faulted,
};

class PortalServer {
public:
    PortalServer(ProvisioningCoordinator& coordinator, IWifiDriver& wifiDriver);
    ~PortalServer();

    bool begin();
    void end();
    void tick(uint32_t now);
    PortalRuntimeState state() const;
    bool httpRunning() const;
    bool dnsRunning() const;
#if defined(UNIT_TEST)
    uint16_t httpStartCount() const;
    uint16_t httpStopCount() const;
    uint16_t dnsStartCount() const;
    uint16_t dnsStopCount() const;
#endif

private:
    class Impl;
    ProvisioningCoordinator& coordinator_;
    IWifiDriver& wifiDriver_;
    std::unique_ptr<Impl> impl_;
};

} // namespace ewfm
