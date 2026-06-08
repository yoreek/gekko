#pragma once

#include "config/ConfigStore.h"
#include "config/DeviceConfig.h"
#include "core/StateMachine.h"
#include "wifi/WifiDriver.h"

#if defined(ARDUINO) && !defined(UNIT_TEST)
#include <WiFi.h>
#endif

namespace ewfm {

enum class WifiManagerResult {
    Accepted,
    InvalidInput,
    StorageError,
    Busy,
};

class WifiManager : public StateMachine {
public:
    explicit WifiManager(IWifiDriver& driver, ConfigStore* configStore = nullptr);

    void begin(const DeviceConfig& config);
    [[nodiscard]] WifiManagerResult submitCredentials(const WiFiCredentials& credentials);
    [[nodiscard]] bool requestBleConfig();

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
    bool bleConfigMode() const {
        return isBleConfigState();
    }
    bool bleConfigRunning() const {
        return is((PState)&WifiManager::BleConfigRunning);
    }
    bool networkStackReady() const {
        return driver_.networkStackReady();
    }
    bool wifiInterfaceUp() const {
        return networkStackReady();
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
#if defined(UNIT_TEST)
    void simulateBleCredentialsReceived(const char* ssid, const char* password);
    void simulateBleProvisioningEnded();
    void setBleStopAutoEnd(bool enabled);
    uint16_t bleStartCount() const {
        return bleStartCount_;
    }
    uint16_t bleStopCount() const {
        return bleStopCount_;
    }
    uint16_t bleDeinitCount() const {
        return bleDeinitCount_;
    }
#endif

private:
    void updateCredentials(const WiFiCredentials& credentials);
    void Idle();
    void Connecting();
    void CheckConnection();
    void RetryDelay();
    void Connected();
    void SetupAp();
    void StartBleConfig();
    void BleConfigRunning();
    void StopBleConfig();
    void WaitBleConfigStopped();
    void DeinitBleConfig();
    void ApplyBleCredentials();
    void ReturnFromBleConfig();
    void ApplySubmittedCredentials();
    bool retriesExhausted() const;
    [[nodiscard]] bool isBleConfigState() const;
    [[nodiscard]] bool pendingCredentialsSubmit() const;
    [[nodiscard]] bool bleConfigRequested() const;
    [[nodiscard]] bool bleCredentialsReceived() const;
    [[nodiscard]] bool bleProvisioningStopped() const;
    [[nodiscard]] WifiManagerResult applyCredentialsNow(const WiFiCredentials& credentials);
    bool takePendingSubmittedCredentials(WiFiCredentials& credentials);
    bool trySavePendingSubmittedCredentials(const WiFiCredentials& credentials);
    bool tryRequestBleConfig();
    void setBleConfigRequested(bool requested);
    void setBleProvisioningStopped(bool stopped);
    void startSetupAp();
    std::string setupApSsid() const;
    [[nodiscard]] bool startBleProvisioning();
    void stopBleProvisioning();
    void deinitBleProvisioning();
    void saveReceivedBleCredentials(const char* ssid, const char* password);
    bool takeReceivedBleCredentials(WiFiCredentials& credentials);
    void clearReceivedBleCredentials();
    void registerWiFiEvents();
#if defined(ARDUINO) && !defined(UNIT_TEST)
    void handleWiFiEvent(arduino_event_id_t eventId, const arduino_event_info_t& info);
#endif

    IWifiDriver& driver_;
    ConfigStore* configStore_{nullptr};
    DeviceConfig config_;
    WiFiCredentials credentials_;
    char pendingSubmitSsid_[kMaxSsidLength + 1]{};
    char pendingSubmitPassword_[kMaxPasswordLength + 1]{};
    char receivedBleSsid_[kMaxSsidLength + 1]{};
    char receivedBlePassword_[kMaxPasswordLength + 1]{};
    uint8_t retryCount_{0};
    bool stationIpLogged_{false};
    bool pendingCredentialsSubmit_{false};
    bool bleConfigRequested_{false};
    bool bleCredentialsReceived_{false};
    bool bleProvisioningStopped_{false};
    bool bleProvisioningStarted_{false};
#if defined(UNIT_TEST)
    uint16_t bleStartCount_{0};
    uint16_t bleStopCount_{0};
    uint16_t bleDeinitCount_{0};
    bool bleStopAutoEnd_{true};
#endif
};

} // namespace ewfm
