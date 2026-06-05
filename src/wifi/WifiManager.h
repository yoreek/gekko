#pragma once

#include "config/DeviceConfig.h"
#include "core/Clock.h"
#include "core/StateMachine.h"
#include "wifi/WifiDriver.h"

namespace ewfm {

enum class WifiManagerState {
    Idle,
    Connecting,
    Connected,
    ProvisioningFallback,
};

class WifiManager {
public:
    WifiManager(IWifiDriver& driver, IClock& clock);

    void begin(const DeviceConfig& config);
    void tick();
    void connect(const WiFiCredentials& credentials);
    void enterProvisioningFallback();
    void clearCredentials();

    WifiManagerState state() const {
        return state_.state();
    }
    bool connected() const {
        return state_.is(WifiManagerState::Connected);
    }
    uint8_t retryCount() const {
        return retryCount_;
    }
    const WiFiCredentials& credentials() const {
        return credentials_;
    }

private:
    void tickIdle(uint32_t now);
    void tickConnecting(uint32_t now);
    void tickConnected(uint32_t now);
    void tickProvisioningFallback(uint32_t now);
    void retryConnection(uint32_t now, const char* reason);
    bool retriesExhausted() const;
    std::string setupApSsid() const;

    IWifiDriver& driver_;
    IClock& clock_;
    StateMachine<WifiManagerState> state_{WifiManagerState::Idle};
    DeviceConfig config_;
    WiFiCredentials credentials_;
    uint8_t retryCount_{0};
    uint32_t nextRetryAt_{0};
};

} // namespace ewfm
