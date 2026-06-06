#pragma once

#include "config/DeviceConfig.h"
#include "core/StateMachine.h"
#include "wifi/WifiDriver.h"

namespace ewfm {

class WifiManager : public StateMachine {
public:
    explicit WifiManager(IWifiDriver& driver);

    void begin(const DeviceConfig& config);
    void updateCredentials(const WiFiCredentials& credentials);
    void clearCredentials();

    bool connected() const {
        return is((PState)&WifiManager::Connected);
    }
    bool connecting() const {
        return is((PState)&WifiManager::Connecting);
    }
    bool apMode() const {
        return is((PState)&WifiManager::SetupAp);
    }
    bool checkConnection() const {
        return is((PState)&WifiManager::CheckConnection);
    }
    bool networkStackReady() const {
        return driver_.networkStackReady();
    }
    bool stationReady() const {
        return connected() && driver_.stationReady();
    }
    bool setupApReady() const {
        return apMode() && driver_.setupApReady();
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
    void CheckConnection();
    void RetryDelay();
    void Connected();
    void SetupAp();
    bool retriesExhausted() const;
    std::string setupApSsid() const;

    IWifiDriver& driver_;
    DeviceConfig config_;
    WiFiCredentials credentials_;
    uint8_t retryCount_{0};
    bool stationIpLogged_{false};
    bool stationConnectRequested_{false};
};

} // namespace ewfm
