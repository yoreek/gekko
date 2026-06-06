#pragma once

#include "config/DeviceConfig.h"
#include "core/Clock.h"
#include "core/StateMachine.h"
#include "wifi/WifiDriver.h"

namespace ewfm {

class WifiManager : public StateMachine {
public:
    WifiManager(IWifiDriver& driver, IClock& clock);

    void begin(const DeviceConfig& config);
    void connect(const WiFiCredentials& credentials);
    void connectAt(const WiFiCredentials& credentials, uint32_t now);
    void enterProvisioningFallback();
    void enterProvisioningFallbackAt(uint32_t now);
    void clearCredentials();
    void clearCredentialsAt(uint32_t now);

    bool connected() const {
        return is((PState)&WifiManager::Connected);
    }
    bool connecting() const {
        return is((PState)&WifiManager::Connecting);
    }
    bool provisioningFallback() const {
        return is((PState)&WifiManager::ProvisioningFallback);
    }
    bool networkStackReady() const {
        return driver_.networkStackReady();
    }
    bool stationReady() const {
        return connected() && driver_.stationReady();
    }
    bool setupApReady() const {
        return provisioningFallback() && driver_.setupApReady();
    }
    bool otaReady() const {
        return stationReady() || setupApReady();
    }
    std::string stationIp() const {
        return driver_.stationIp();
    }
    std::string otaIp() const {
        if (stationReady()) {
            return driver_.stationIp();
        }
        if (setupApReady()) {
            return driver_.setupApIp();
        }
        return {};
    }
    uint8_t retryCount() const {
        return retryCount_;
    }
    const WiFiCredentials& credentials() const {
        return credentials_;
    }

private:
    void Idle();
    void Connecting();
    void Connected();
    void ProvisioningFallback();
    void startProvisioningFallbackAt(uint32_t now);
    void retryConnection(uint32_t now, const char* reason);
    bool retriesExhausted() const;
    std::string setupApSsid() const;

    IWifiDriver& driver_;
    IClock& clock_;
    DeviceConfig config_;
    WiFiCredentials credentials_;
    uint8_t retryCount_{0};
    uint32_t nextRetryAt_{0};
    bool stationIpLogged_{false};
};

} // namespace ewfm
