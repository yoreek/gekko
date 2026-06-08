#include "wifi/WifiManager.h"

#include "debug/Debug.h"

#include <cstring>

#if defined(ARDUINO) && !defined(UNIT_TEST)
#include <WiFi.h>
#include <WiFiProv.h>
#include <wifi_provisioning/manager.h>
#include <wifi_provisioning/scheme_ble.h>
#endif

namespace ewfm {

#undef SM_CLASS
#define SM_CLASS WifiManager

#if defined(ARDUINO) && !defined(UNIT_TEST)
namespace {
WifiManager* activeWifiManager = nullptr;
bool wifiEventsRegistered = false;
portMUX_TYPE controlMutex = portMUX_INITIALIZER_UNLOCKED;
portMUX_TYPE bleEventMutex = portMUX_INITIALIZER_UNLOCKED;

constexpr uint8_t kBleProvisioningUuid[16] = {0xb4, 0xdf, 0x5a, 0x1c, 0x3f, 0x6b, 0xf4, 0xbf,
                                              0xea, 0x4a, 0x82, 0x03, 0x04, 0x90, 0x1a, 0x02};
} // namespace
#endif

namespace {
void copyBoundedCString(char* destination, const size_t destinationSize, const char* value) {
    if (destination == nullptr || destinationSize == 0) {
        return;
    }
    if (value == nullptr) {
        destination[0] = '\0';
        return;
    }

    size_t index = 0;
    while (index + 1 < destinationSize && value[index] != '\0') {
        destination[index] = value[index];
        ++index;
    }
    destination[index] = '\0';
}
} // namespace

constexpr uint32_t kBleStopTimeoutMs = 5000;

WifiManager::WifiManager(IWifiDriver& driver, ConfigStore* configStore)
    : StateMachine((PState)&WifiManager::Idle), driver_(driver), configStore_(configStore) {}

void WifiManager::begin(const DeviceConfig& config) {
    driver_.begin();
    registerWiFiEvents();
    config_ = config;
    credentials_ = config.wifi;
    retryCount_ = 0;
    stationIpLogged_ = false;
    pendingCredentialsSubmit_ = false;
    setBleConfigRequested(false);
    clearReceivedBleCredentials();
    setBleProvisioningStopped(false);
    bleProvisioningStarted_ = false;
}

WifiManagerResult WifiManager::submitCredentials(const WiFiCredentials& credentials) {
    if (isBleConfigState()) {
        EWFM_WIFI_LOG_WARN("credentials rejected: BLE config mode is active");
        return WifiManagerResult::Busy;
    }

    ValidationResult validation = validateWifiCredentials(credentials, true);
    if (!validation.ok()) {
        EWFM_WIFI_LOG_WARN("credentials rejected: %s", validation.message);
        return WifiManagerResult::InvalidInput;
    }

    if (!trySavePendingSubmittedCredentials(credentials)) {
        EWFM_WIFI_LOG_WARN("credentials rejected: control operation pending");
        return WifiManagerResult::Busy;
    }

    EWFM_WIFI_LOG_INFO("credentials accepted");
    return WifiManagerResult::Accepted;
}

bool WifiManager::requestBleConfig() {
    if (!config_.provisioning.mobileProvisioningEnabled) {
        EWFM_WIFI_LOG_WARN("BLE config mode rejected: disabled");
        return false;
    }

    if (isBleConfigState()) {
        EWFM_WIFI_LOG_WARN("BLE config mode rejected: already active");
        return false;
    }

    if (!tryRequestBleConfig()) {
        EWFM_WIFI_LOG_WARN("BLE config mode rejected: control operation pending");
        return false;
    }

    EWFM_WIFI_LOG_INFO("BLE config mode requested");
    return true;
}

void WifiManager::updateCredentials(const WiFiCredentials& credentials) {
    credentials_ = credentials;
    retryCount_ = 0;
    stationIpLogged_ = false;
}

SM_STATE(Idle) {
    if (credentials_.hasCredentials()) {
        EWFM_WIFI_LOG_INFO("stored credentials found, connecting");
        SM_GOTO(Connecting);
    }

    EWFM_WIFI_LOG_INFO("starting setup AP");
    SM_GOTO(SetupAp);
}

SM_STATE(Connecting) {
    driver_.beginStation(credentials_);
    EWFM_WIFI_LOG_INFO("station connection started for ssid=%s", credentials_.ssid.c_str());
    SM_GOTO(CheckConnection);
}

SM_STATE(CheckConnection) {
    if (bleConfigRequested()) {
        EWFM_WIFI_LOG_INFO("BLE config requested, leaving connection check");
        SM_GOTO(StartBleConfig);
    }

    if (pendingCredentialsSubmit()) {
        EWFM_WIFI_LOG_INFO("submitted credentials pending during connection check");
        SM_GOTO(ApplySubmittedCredentials);
    }

    if (driver_.status() == WifiDriverStatus::Connected) {
        EWFM_WIFI_LOG_INFO("station connected");
        stationIpLogged_ = false;
        SM_GOTO(Connected);
    }

    if (isTimeout(config_.wifiRuntime.connectTimeoutMs)) {
        stationIpLogged_ = false;
        ++retryCount_;
        EWFM_WIFI_LOG_DEBUG("connection timeout retry %u", retryCount_);
        if (retriesExhausted()) {
            SM_GOTO(SetupAp);
        }
        SM_GOTO(RetryDelay);
    }
}

SM_STATE(RetryDelay) {
    if (bleConfigRequested()) {
        EWFM_WIFI_LOG_INFO("BLE config requested, leaving retry delay");
        SM_GOTO(StartBleConfig);
    }

    if (isTimeout(config_.wifiRuntime.retryDelayMs)) {
        SM_GOTO(Idle);
    }
}

SM_STATE(Connected) {
    if (bleConfigRequested()) {
        EWFM_WIFI_LOG_INFO("BLE config requested, leaving connected station mode");
        SM_GOTO(StartBleConfig);
    }

    if (!stationIpLogged_) {
        const std::string ip = driver_.stationIp();
        if (!ip.empty()) {
            EWFM_WIFI_LOG_INFO("station ip=%s", ip.c_str());
            stationIpLogged_ = true;
        }
    }

    if (driver_.status() != WifiDriverStatus::Connected) {
        stationIpLogged_ = false;
        retryCount_ = 0;
        EWFM_WIFI_LOG_WARN("station disconnected, returning to idle");
        SM_GOTO(Idle);
    }
}

SM_STATE(SetupAp) {
    if (isStateUpdated()) {
        startSetupAp();
    }

    if (bleConfigRequested()) {
        EWFM_WIFI_LOG_INFO("BLE config requested from setup AP");
        SM_GOTO(StartBleConfig);
    }

    if (pendingCredentialsSubmit()) {
        EWFM_WIFI_LOG_INFO("submitted credentials pending from setup AP");
        SM_GOTO(ApplySubmittedCredentials);
    }
}

SM_STATE(StartBleConfig) {
    setBleConfigRequested(false);
    clearReceivedBleCredentials();
    setBleProvisioningStopped(false);
    bleProvisioningStarted_ = false;
    EWFM_WIFI_LOG_INFO("BLE config session timeout=%lu ms", static_cast<unsigned long>(config_.provisioning.sessionTimeoutMs));

    if (!startBleProvisioning()) {
        EWFM_WIFI_LOG_WARN("BLE config start failed");
        SM_GOTO(ReturnFromBleConfig);
    }
    bleProvisioningStarted_ = true;

    SM_GOTO(BleConfigRunning);
}

SM_STATE(BleConfigRunning) {
    if (bleCredentialsReceived()) {
        EWFM_WIFI_LOG_INFO("BLE credentials received, applying credentials");
        SM_GOTO(ApplyBleCredentials);
    }

    if (isTimeout(config_.provisioning.sessionTimeoutMs)) {
        EWFM_WIFI_LOG_INFO("BLE config mode timed out");
        SM_GOTO(StopBleConfig);
    }
}

SM_STATE(StopBleConfig) {
    stopBleProvisioning();
    SM_GOTO(WaitBleConfigStopped);
}

SM_STATE(WaitBleConfigStopped) {
    if (bleProvisioningStopped()) {
        SM_GOTO(DeinitBleConfig);
    }

    if (isTimeout(kBleStopTimeoutMs)) {
        EWFM_WIFI_LOG_WARN("BLE config stop timed out, deinitializing");
        SM_GOTO(DeinitBleConfig);
    }
}

SM_STATE(DeinitBleConfig) {
    deinitBleProvisioning();
    bleProvisioningStarted_ = false;
    SM_GOTO(Idle);
}

SM_STATE(ApplyBleCredentials) {
    WiFiCredentials credentials;
    if (takeReceivedBleCredentials(credentials)) {
        WifiManagerResult result = applyCredentialsNow(credentials);
        if (result != WifiManagerResult::Accepted) {
            EWFM_WIFI_LOG_WARN("BLE credentials were not applied");
        }
    }

    SM_GOTO(StopBleConfig);
}

SM_STATE(ApplySubmittedCredentials) {
    WiFiCredentials credentials;
    if (!takePendingSubmittedCredentials(credentials)) {
        SM_GOTO(Idle);
    }

    WifiManagerResult result = applyCredentialsNow(credentials);
    if (result != WifiManagerResult::Accepted) {
        EWFM_WIFI_LOG_WARN("submitted credentials were not applied");
        SM_GOTO(Idle);
    }

    SM_GOTO(Idle);
}

SM_STATE(ReturnFromBleConfig) {
    setBleConfigRequested(false);
    setBleProvisioningStopped(false);
    bleProvisioningStarted_ = false;
    clearReceivedBleCredentials();
    SM_GOTO(Idle);
}

bool WifiManager::retriesExhausted() const {
    return retryCount_ >= config_.wifiRuntime.maxConnectRetries;
}

bool WifiManager::isBleConfigState() const {
    return is((PState)&WifiManager::StartBleConfig) || is((PState)&WifiManager::BleConfigRunning) ||
           is((PState)&WifiManager::StopBleConfig) || is((PState)&WifiManager::WaitBleConfigStopped) ||
           is((PState)&WifiManager::DeinitBleConfig) || is((PState)&WifiManager::ApplyBleCredentials) ||
           is((PState)&WifiManager::ReturnFromBleConfig);
}

bool WifiManager::pendingCredentialsSubmit() const {
#if defined(ARDUINO) && !defined(UNIT_TEST)
    portENTER_CRITICAL(&controlMutex);
#endif
    const bool pending = pendingCredentialsSubmit_;
#if defined(ARDUINO) && !defined(UNIT_TEST)
    portEXIT_CRITICAL(&controlMutex);
#endif
    return pending;
}

bool WifiManager::bleConfigRequested() const {
#if defined(ARDUINO) && !defined(UNIT_TEST)
    portENTER_CRITICAL(&controlMutex);
#endif
    const bool requested = bleConfigRequested_;
#if defined(ARDUINO) && !defined(UNIT_TEST)
    portEXIT_CRITICAL(&controlMutex);
#endif
    return requested;
}

bool WifiManager::bleCredentialsReceived() const {
#if defined(ARDUINO) && !defined(UNIT_TEST)
    portENTER_CRITICAL(&bleEventMutex);
#endif
    const bool received = bleCredentialsReceived_;
#if defined(ARDUINO) && !defined(UNIT_TEST)
    portEXIT_CRITICAL(&bleEventMutex);
#endif
    return received;
}

bool WifiManager::bleProvisioningStopped() const {
#if defined(ARDUINO) && !defined(UNIT_TEST)
    portENTER_CRITICAL(&bleEventMutex);
#endif
    const bool stopped = bleProvisioningStopped_;
#if defined(ARDUINO) && !defined(UNIT_TEST)
    portEXIT_CRITICAL(&bleEventMutex);
#endif
    return stopped;
}

WifiManagerResult WifiManager::applyCredentialsNow(const WiFiCredentials& credentials) {
    ValidationResult validation =
        configStore_ != nullptr ? configStore_->saveWifiCredentials(credentials) : validateWifiCredentials(credentials, true);
    if (!validation.ok()) {
        EWFM_WIFI_LOG_WARN("credentials rejected: %s", validation.message);
        return validation.error == ConfigError::StorageError ? WifiManagerResult::StorageError : WifiManagerResult::InvalidInput;
    }

    if (configStore_ != nullptr) {
        config_ = configStore_->config();
    } else {
        config_.wifi = credentials;
    }

    EWFM_WIFI_LOG_INFO("credentials applied");
    updateCredentials(credentials);
    return WifiManagerResult::Accepted;
}

bool WifiManager::takePendingSubmittedCredentials(WiFiCredentials& credentials) {
    char ssid[kMaxSsidLength + 1]{};
    char password[kMaxPasswordLength + 1]{};
    bool pending = false;

#if defined(ARDUINO) && !defined(UNIT_TEST)
    portENTER_CRITICAL(&controlMutex);
#endif
    pending = pendingCredentialsSubmit_;
    if (pending) {
        std::memcpy(ssid, pendingSubmitSsid_, sizeof(ssid));
        std::memcpy(password, pendingSubmitPassword_, sizeof(password));
        pendingCredentialsSubmit_ = false;
        pendingSubmitSsid_[0] = '\0';
        pendingSubmitPassword_[0] = '\0';
    }
#if defined(ARDUINO) && !defined(UNIT_TEST)
    portEXIT_CRITICAL(&controlMutex);
#endif

    if (!pending) {
        return false;
    }

    credentials.ssid = ssid;
    credentials.password = password;
    return true;
}

bool WifiManager::trySavePendingSubmittedCredentials(const WiFiCredentials& credentials) {
#if defined(ARDUINO) && !defined(UNIT_TEST)
    portENTER_CRITICAL(&controlMutex);
#endif
    const bool accepted = !pendingCredentialsSubmit_ && !bleConfigRequested_;
    if (accepted) {
        copyBoundedCString(pendingSubmitSsid_, sizeof(pendingSubmitSsid_), credentials.ssid.c_str());
        copyBoundedCString(pendingSubmitPassword_, sizeof(pendingSubmitPassword_), credentials.password.c_str());
        pendingCredentialsSubmit_ = true;
    }
#if defined(ARDUINO) && !defined(UNIT_TEST)
    portEXIT_CRITICAL(&controlMutex);
#endif
    return accepted;
}

bool WifiManager::tryRequestBleConfig() {
#if defined(ARDUINO) && !defined(UNIT_TEST)
    portENTER_CRITICAL(&controlMutex);
#endif
    const bool accepted = !bleConfigRequested_ && !pendingCredentialsSubmit_;
    if (accepted) {
        bleConfigRequested_ = true;
    }
#if defined(ARDUINO) && !defined(UNIT_TEST)
    portEXIT_CRITICAL(&controlMutex);
#endif
    return accepted;
}

void WifiManager::setBleConfigRequested(bool requested) {
#if defined(ARDUINO) && !defined(UNIT_TEST)
    portENTER_CRITICAL(&controlMutex);
#endif
    bleConfigRequested_ = requested;
#if defined(ARDUINO) && !defined(UNIT_TEST)
    portEXIT_CRITICAL(&controlMutex);
#endif
}

void WifiManager::setBleProvisioningStopped(bool stopped) {
#if defined(ARDUINO) && !defined(UNIT_TEST)
    portENTER_CRITICAL(&bleEventMutex);
#endif
    bleProvisioningStopped_ = stopped;
#if defined(ARDUINO) && !defined(UNIT_TEST)
    portEXIT_CRITICAL(&bleEventMutex);
#endif
}

void WifiManager::startSetupAp() {
    if (driver_.startSetupAp(setupApSsid(), config_.provisioning.setupApPassword)) {
        EWFM_WIFI_LOG_INFO("setup AP started");
    } else {
        EWFM_WIFI_LOG_WARN("setup AP start failed");
    }
}

std::string WifiManager::setupApSsid() const {
    std::string suffix = driver_.macSuffix();
    if (suffix.empty()) {
        suffix = "setup";
    }
    return config_.deviceName + "-" + suffix;
}

bool WifiManager::startBleProvisioning() {
#if defined(UNIT_TEST)
    ++bleStartCount_;
    return true;
#elif defined(ARDUINO)
    EWFM_WIFI_LOG_INFO("BLE config start service=%s", setupApSsid().c_str());

    wifi_prov_mgr_config_t provConfig{};
    provConfig.scheme = wifi_prov_scheme_ble;
    wifi_prov_event_handler_t schemeHandler = WIFI_PROV_EVENT_HANDLER_NONE;
    std::memcpy(&provConfig.scheme_event_handler, &schemeHandler, sizeof(wifi_prov_event_handler_t));
    provConfig.app_event_handler.event_cb = nullptr;
    provConfig.app_event_handler.user_data = nullptr;

    uint8_t uuid[sizeof(kBleProvisioningUuid)]{};
    std::memcpy(uuid, kBleProvisioningUuid, sizeof(uuid));
    wifi_prov_scheme_ble_set_service_uuid(uuid);

    if (wifi_prov_mgr_init(provConfig) != ESP_OK) {
        EWFM_WIFI_LOG_WARN("wifi_prov_mgr_init failed");
        return false;
    }

    wifi_prov_mgr_reset_provisioning();
    if (wifi_prov_mgr_start_provisioning(WIFI_PROV_SECURITY_1, config_.provisioning.proofOfPossession.c_str(), setupApSsid().c_str(),
                                         nullptr) != ESP_OK) {
        EWFM_WIFI_LOG_WARN("wifi_prov_mgr_start_provisioning failed");
        wifi_prov_mgr_deinit();
        return false;
    }
    return true;
#else
    return false;
#endif
}

void WifiManager::stopBleProvisioning() {
#if defined(UNIT_TEST)
    ++bleStopCount_;
    if (bleStopAutoEnd_) {
        setBleProvisioningStopped(true);
    }
#elif defined(ARDUINO)
    EWFM_WIFI_LOG_INFO("BLE config stop requested");
    wifi_prov_mgr_stop_provisioning();
#endif
}

void WifiManager::deinitBleProvisioning() {
#if defined(UNIT_TEST)
    ++bleDeinitCount_;
#elif defined(ARDUINO)
    EWFM_WIFI_LOG_INFO("BLE config deinit");
    wifi_prov_mgr_deinit();
#endif
}

void WifiManager::saveReceivedBleCredentials(const char* ssid, const char* password) {
#if defined(ARDUINO) && !defined(UNIT_TEST)
    portENTER_CRITICAL(&bleEventMutex);
#endif
    copyBoundedCString(receivedBleSsid_, sizeof(receivedBleSsid_), ssid);
    copyBoundedCString(receivedBlePassword_, sizeof(receivedBlePassword_), password);
    bleCredentialsReceived_ = true;
#if defined(ARDUINO) && !defined(UNIT_TEST)
    portEXIT_CRITICAL(&bleEventMutex);
#endif
}

bool WifiManager::takeReceivedBleCredentials(WiFiCredentials& credentials) {
    char ssid[kMaxSsidLength + 1]{};
    char password[kMaxPasswordLength + 1]{};
    bool received = false;

#if defined(ARDUINO) && !defined(UNIT_TEST)
    portENTER_CRITICAL(&bleEventMutex);
#endif
    received = bleCredentialsReceived_;
    if (received) {
        std::memcpy(ssid, receivedBleSsid_, sizeof(ssid));
        std::memcpy(password, receivedBlePassword_, sizeof(password));
        bleCredentialsReceived_ = false;
        receivedBleSsid_[0] = '\0';
        receivedBlePassword_[0] = '\0';
    }
#if defined(ARDUINO) && !defined(UNIT_TEST)
    portEXIT_CRITICAL(&bleEventMutex);
#endif

    if (!received) {
        return false;
    }

    credentials.ssid = ssid;
    credentials.password = password;
    return true;
}

void WifiManager::clearReceivedBleCredentials() {
#if defined(ARDUINO) && !defined(UNIT_TEST)
    portENTER_CRITICAL(&bleEventMutex);
#endif
    bleCredentialsReceived_ = false;
    receivedBleSsid_[0] = '\0';
    receivedBlePassword_[0] = '\0';
#if defined(ARDUINO) && !defined(UNIT_TEST)
    portEXIT_CRITICAL(&bleEventMutex);
#endif
}

void WifiManager::registerWiFiEvents() {
#if defined(ARDUINO) && !defined(UNIT_TEST)
    activeWifiManager = this;
    if (wifiEventsRegistered) {
        return;
    }

    WiFi.onEvent([](const arduino_event_id_t eventId, const arduino_event_info_t& info) {
        if (activeWifiManager != nullptr) {
            activeWifiManager->handleWiFiEvent(eventId, info);
        }
    });
    wifiEventsRegistered = true;
#endif
}

#if defined(ARDUINO) && !defined(UNIT_TEST)
void WifiManager::handleWiFiEvent(const arduino_event_id_t eventId, const arduino_event_info_t& info) {
    switch (eventId) {
    case ARDUINO_EVENT_PROV_INIT:
        EWFM_WIFI_LOG_INFO("event PROV_INIT");
        break;
    case ARDUINO_EVENT_PROV_DEINIT:
        EWFM_WIFI_LOG_INFO("event PROV_DEINIT");
        break;
    case ARDUINO_EVENT_PROV_START:
        EWFM_WIFI_LOG_INFO("event PROV_START");
        break;
    case ARDUINO_EVENT_PROV_END:
        EWFM_WIFI_LOG_INFO("event PROV_END");
        setBleProvisioningStopped(true);
        break;
    case ARDUINO_EVENT_PROV_CRED_RECV:
        EWFM_WIFI_LOG_INFO("event PROV_CRED_RECV ssid=%s", reinterpret_cast<const char*>(info.prov_cred_recv.ssid));
        saveReceivedBleCredentials(reinterpret_cast<const char*>(info.prov_cred_recv.ssid),
                                   reinterpret_cast<const char*>(info.prov_cred_recv.password));
        break;
    case ARDUINO_EVENT_PROV_CRED_FAIL:
        EWFM_WIFI_LOG_WARN("event PROV_CRED_FAIL reason=%d", static_cast<int>(info.prov_fail_reason));
        break;
    case ARDUINO_EVENT_PROV_CRED_SUCCESS:
        EWFM_WIFI_LOG_INFO("event PROV_CRED_SUCCESS");
        break;
    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
        EWFM_WIFI_LOG_INFO("event WIFI_STA_DISCONNECTED reason=%u", static_cast<unsigned>(info.wifi_sta_disconnected.reason));
        break;
    case ARDUINO_EVENT_WIFI_STA_CONNECTED:
        EWFM_WIFI_LOG_INFO("event WIFI_STA_CONNECTED");
        break;
    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
        EWFM_WIFI_LOG_INFO("event WIFI_STA_GOT_IP");
        break;
    case ARDUINO_EVENT_WIFI_STA_START:
        EWFM_WIFI_LOG_INFO("event WIFI_STA_START");
        break;
    case ARDUINO_EVENT_WIFI_STA_STOP:
        EWFM_WIFI_LOG_INFO("event WIFI_STA_STOP");
        break;
    default:
        break;
    }
}
#endif

#if defined(UNIT_TEST)
void WifiManager::simulateBleCredentialsReceived(const char* ssid, const char* password) {
    saveReceivedBleCredentials(ssid, password);
}

void WifiManager::simulateBleProvisioningEnded() {
    setBleProvisioningStopped(true);
}

void WifiManager::setBleStopAutoEnd(bool enabled) {
    bleStopAutoEnd_ = enabled;
}
#endif

} // namespace ewfm
