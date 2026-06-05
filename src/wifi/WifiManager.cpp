#include "wifi/WifiManager.h"

#include "debug/Debug.h"

namespace ewfm {

WifiManager::WifiManager(IWifiDriver& driver, IClock& clock) : driver_(driver), clock_(clock) {}

void WifiManager::begin(const DeviceConfig& config) {
    config_ = config;
    credentials_ = config.wifi;
    retryCount_ = 0;

    if (credentials_.hasCredentials()) {
        EWFM_WIFI_LOG_INFO("stored credentials found, connecting");
        connect(credentials_);
    } else if (config_.wifiRuntime.fallbackApEnabled) {
        EWFM_WIFI_LOG_INFO("no credentials, entering provisioning fallback");
        enterProvisioningFallback();
    }
}

void WifiManager::connect(const WiFiCredentials& credentials) {
    credentials_ = credentials;
    retryCount_ = 0;
    nextRetryAt_ = clock_.millis();
    driver_.beginStation(credentials_);
    EWFM_WIFI_LOG_INFO("station connection started for ssid=%s", credentials.ssid.c_str());
    state_.transitionTo(WifiManagerState::Connecting, clock_.millis());
}

void WifiManager::enterProvisioningFallback() {
    driver_.disconnect();
    driver_.startSetupAp(setupApSsid(), config_.provisioning.setupApPassword);
    EWFM_WIFI_LOG_INFO("setup AP started");
    state_.transitionTo(WifiManagerState::ProvisioningFallback, clock_.millis());
}

void WifiManager::clearCredentials() {
    credentials_ = {};
    enterProvisioningFallback();
}

void WifiManager::tick() {
    if (state_.isPaused()) {
        return;
    }

    const uint32_t now = clock_.millis();

    switch (state_.state()) {
    case WifiManagerState::Idle:
        tickIdle(now);
        return;
    case WifiManagerState::Connecting:
        tickConnecting(now);
        return;
    case WifiManagerState::Connected:
        tickConnected(now);
        return;
    case WifiManagerState::ProvisioningFallback:
        tickProvisioningFallback(now);
        return;
    }
}

void WifiManager::tickIdle(uint32_t now) {
    (void)now;
}

void WifiManager::tickConnecting(uint32_t now) {
    switch (driver_.status()) {
    case WifiDriverStatus::Connected:
        driver_.stopSetupAp();
        EWFM_WIFI_LOG_INFO("station connected");
        EWFM_SM_GOTO(state_, WifiManagerState::Connected, now);
    case WifiDriverStatus::Failed:
    case WifiDriverStatus::Disconnected:
        if (retriesExhausted()) {
            if (config_.wifiRuntime.fallbackApEnabled) {
                EWFM_WIFI_LOG_WARN("connection failed, entering provisioning fallback");
                enterProvisioningFallback();
            }
            return;
        }
        if (EWFM_SM_TIME_REACHED(now, nextRetryAt_)) {
            retryConnection(now, "connection retry");
        }
        return;
    case WifiDriverStatus::Connecting:
    case WifiDriverStatus::Idle:
        if (EWFM_SM_TIMEOUT(state_, now, config_.wifiRuntime.connectTimeoutMs)) {
            if (retriesExhausted()) {
                enterProvisioningFallback();
            } else {
                retryConnection(now, "connection timeout retry");
            }
        }
        return;
    }
}

void WifiManager::tickConnected(uint32_t now) {
    (void)now;

    if (driver_.status() == WifiDriverStatus::Disconnected || driver_.status() == WifiDriverStatus::Failed) {
        EWFM_WIFI_LOG_WARN("station disconnected, reconnecting");
        retryConnection(clock_.millis(), "connection restore retry");
    }
}

void WifiManager::tickProvisioningFallback(uint32_t now) {
    (void)now;
}

void WifiManager::retryConnection(uint32_t now, const char* reason) {
    (void)reason;
    ++retryCount_;
    EWFM_WIFI_LOG_DEBUG("%s %u", reason, retryCount_);
    nextRetryAt_ = now + config_.wifiRuntime.retryDelayMs;
    driver_.beginStation(credentials_);
    if (state_.is(WifiManagerState::Connecting)) {
        state_.resetTimer(now);
    } else {
        state_.transitionTo(WifiManagerState::Connecting, now);
    }
}

bool WifiManager::retriesExhausted() const {
    return retryCount_ >= config_.wifiRuntime.maxConnectRetries;
}

std::string WifiManager::setupApSsid() const {
    std::string suffix = driver_.macSuffix();
    if (suffix.empty()) {
        suffix = "setup";
    }
    return config_.deviceName + "-" + suffix;
}

} // namespace ewfm
