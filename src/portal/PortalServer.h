#pragma once

#include "devices/registry/DeviceRegistry.h"
#include "integrations/common/DeviceEventDispatcher.h"
#include "portal/DashboardLayoutStore.h"
#include "wifi/WifiDriver.h"
#include "wifi/WifiManager.h"

#include <cstdint>
#include <memory>

namespace ewfm {

class MqttConfigStore;
class MqttManager;
class HaDiscoveryBridge;
class NtpManager;
class ConfigStore;

enum class PortalRuntimeState {
    Idle,
    WaitingForNetwork,
    Starting,
    Running,
    Faulted,
};

class PortalServer {
public:
    PortalServer(WifiManager& wifiManager, IWifiDriver& wifiDriver, DeviceRegistry* deviceRegistry = nullptr,
                 DeviceEventDispatcher* deviceEventDispatcher = nullptr, DashboardLayoutStore* dashboardLayoutStore = nullptr,
                 MqttConfigStore* mqttConfigStore = nullptr, MqttManager* mqttManager = nullptr,
                 DeviceScopedDataStore* haSettingsStore = nullptr, HaDiscoveryBridge* haDiscoveryBridge = nullptr,
                 NtpManager* ntpManager = nullptr, ConfigStore* configStore = nullptr);
    // NOLINTNEXTLINE(performance-trivially-destructible)
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
    WifiManager& wifiManager_;
    IWifiDriver& wifiDriver_;
    DeviceRegistry* deviceRegistry_{nullptr};
    DeviceEventDispatcher* deviceEventDispatcher_{nullptr};
    DashboardLayoutStore* dashboardLayoutStore_{nullptr};
    MqttConfigStore* mqttConfigStore_{nullptr};
    MqttManager* mqttManager_{nullptr};
    DeviceScopedDataStore* haSettingsStore_{nullptr};
    HaDiscoveryBridge* haDiscoveryBridge_{nullptr};
    NtpManager* ntpManager_{nullptr};
    ConfigStore* configStore_{nullptr};
    std::unique_ptr<Impl> impl_;
};

} // namespace ewfm
