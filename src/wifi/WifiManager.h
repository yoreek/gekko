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
    [[nodiscard]] WifiManagerResult clearCredentials();
#if defined(WITH_BLE_PROVISIONING)
    [[nodiscard]] bool requestBleConfig();
#else
    [[nodiscard]] constexpr bool requestBleConfig() {
        return false;
    }
#endif
    static constexpr bool bleProvisioningSupported() {
#if defined(WITH_BLE_PROVISIONING)
        return true;
#else
        return false;
#endif
    }

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
#if defined(WITH_BLE_PROVISIONING)
        return isBleConfigState();
#else
        return false;
#endif
    }
    bool bleConfigRunning() const {
#if defined(WITH_BLE_PROVISIONING)
        return is((PState)&WifiManager::BleConfigRunning);
#else
        return false;
#endif
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
    int32_t rssi() const {
        return driver_.rssi();
    }
    std::string ssid() const {
        return driver_.ssid();
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
#if defined(UNIT_TEST) && defined(WITH_BLE_PROVISIONING)
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
#if defined(WITH_BLE_PROVISIONING)
    void StartBleConfig();
    void BleConfigRunning();
    void StopBleConfig();
    void WaitBleConfigStopped();
    void DeinitBleConfig();
    void ApplyBleCredentials();
    void ReturnFromBleConfig();
#endif
    void ApplySubmittedCredentials();
    bool retriesExhausted() const;
#if defined(WITH_BLE_PROVISIONING)
    [[nodiscard]] bool isBleConfigState() const;
#endif
    [[nodiscard]] bool pendingCredentialsSubmit() const;
#if defined(WITH_BLE_PROVISIONING)
    [[nodiscard]] bool bleConfigRequested() const;
    [[nodiscard]] bool bleCredentialsReceived() const;
    [[nodiscard]] bool bleProvisioningStopped() const;
    bool tryRequestBleConfig();
    void setBleConfigRequested(bool requested);
    void setBleProvisioningStopped(bool stopped);
#endif
    [[nodiscard]] WifiManagerResult applyCredentialsNow(const WiFiCredentials& credentials);
    bool takePendingSubmittedCredentials(WiFiCredentials& credentials);
    bool trySavePendingSubmittedCredentials(const WiFiCredentials& credentials);
    void startSetupAp();
    std::string setupApSsid() const;
#if defined(WITH_BLE_PROVISIONING)
    [[nodiscard]] bool startBleProvisioning();
    void stopBleProvisioning();
    void deinitBleProvisioning();
    void saveReceivedBleCredentials(const WiFiCredentials& credentials);
    bool takeReceivedBleCredentials(WiFiCredentials& credentials);
    void clearReceivedBleCredentials();
#endif
    void registerWiFiEvents();

    IWifiDriver& driver_;
    ConfigStore* configStore_{nullptr};
    DeviceConfig config_;
    WiFiCredentials credentials_;
#if defined(ARDUINO) && !defined(UNIT_TEST)
    mutable portMUX_TYPE controlMutex_;
#if defined(WITH_BLE_PROVISIONING)
    mutable portMUX_TYPE bleEventMutex_;
#endif
    bool wifiEventsRegistered_{false};
#endif
    char pendingSubmitSsid_[kMaxSsidLength + 1]{};
    char pendingSubmitPassword_[kMaxPasswordLength + 1]{};
#if defined(WITH_BLE_PROVISIONING)
    char receivedBleSsid_[kMaxSsidLength + 1]{};
    char receivedBlePassword_[kMaxPasswordLength + 1]{};
#endif
    uint8_t retryCount_{0};
    bool stationIpLogged_{false};
    bool pendingCredentialsSubmit_{false};
#if defined(WITH_BLE_PROVISIONING)
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
#endif

#if defined(WITH_BLE_PROVISIONING) && defined(ARDUINO) && !defined(UNIT_TEST)
    inline static constexpr uint8_t kBleProvisioningUuid_[16] = {0xb4, 0xdf, 0x5a, 0x1c, 0x3f, 0x6b, 0xf4, 0xbf,
                                                                 0xea, 0x4a, 0x82, 0x03, 0x04, 0x90, 0x1a, 0x02};
#endif
};

} // namespace ewfm
