#include "wifi/WifiManager.h"

#include "debug/Debug.h"

namespace ewfm {

#undef SM_CLASS
#define SM_CLASS WifiManager

WifiManager::WifiManager(IWifiDriver& driver, IClock& clock) : StateMachine((PState)&WifiManager::Idle), driver_(driver), clock_(clock) {}

void WifiManager::begin(const DeviceConfig& config) {
    config_ = config;
    credentials_ = config.wifi;
    retryCount_ = 0;
    nextRetryAt_ = 0;
}

void WifiManager::connect(const WiFiCredentials& credentials) {
    connectAt(credentials, clock_.millis());
}

void WifiManager::connectAt(const WiFiCredentials& credentials, uint32_t now) {
    credentials_ = credentials;
    retryCount_ = 0;
    nextRetryAt_ = now;
    driver_.beginStation(credentials_);
    EWFM_WIFI_LOG_INFO("station connection started for ssid=%s", credentials.ssid.c_str());
    setState((PState)&WifiManager::Connecting, now);
}

void WifiManager::enterProvisioningFallback() {
    enterProvisioningFallbackAt(clock_.millis());
}

void WifiManager::enterProvisioningFallbackAt(uint32_t now) {
    driver_.disconnect();
    driver_.startSetupAp(setupApSsid(), config_.provisioning.setupApPassword);
    EWFM_WIFI_LOG_INFO("setup AP started");
    setState((PState)&WifiManager::ProvisioningFallback, now);
}

void WifiManager::clearCredentials() {
    credentials_ = {};
    enterProvisioningFallback();
}

void WifiManager::tick(uint32_t now) {
    loop(now);
}

SM_STATE(Idle) {
    if (credentials_.hasCredentials()) {
        EWFM_WIFI_LOG_INFO("stored credentials found, connecting");
        connectAt(credentials_, uptime());
        return;
    }

    if (config_.wifiRuntime.fallbackApEnabled) {
        EWFM_WIFI_LOG_INFO("no credentials, entering provisioning fallback");
        enterProvisioningFallbackAt(uptime());
    }
}

SM_STATE(Connecting) {
    switch (driver_.status()) {
    case WifiDriverStatus::Connected:
        driver_.stopSetupAp();
        EWFM_WIFI_LOG_INFO("station connected");
        SM_GOTO(Connected);
    case WifiDriverStatus::Failed:
    case WifiDriverStatus::Disconnected:
        if (retriesExhausted()) {
            if (config_.wifiRuntime.fallbackApEnabled) {
                EWFM_WIFI_LOG_WARN("connection failed, entering provisioning fallback");
                enterProvisioningFallbackAt(uptime());
            }
            return;
        }
        if (timeReached(uptime(), nextRetryAt_)) {
            retryConnection(uptime(), "connection retry");
        }
        return;
    case WifiDriverStatus::Connecting:
    case WifiDriverStatus::Idle:
        if (isTimeout(config_.wifiRuntime.connectTimeoutMs)) {
            if (retriesExhausted()) {
                enterProvisioningFallbackAt(uptime());
            } else {
                retryConnection(uptime(), "connection timeout retry");
            }
        }
        return;
    }
}

SM_STATE(Connected) {
    if (driver_.status() == WifiDriverStatus::Disconnected || driver_.status() == WifiDriverStatus::Failed) {
        EWFM_WIFI_LOG_WARN("station disconnected, reconnecting");
        retryConnection(uptime(), "connection restore retry");
    }
}

SM_STATE(ProvisioningFallback) {}

void WifiManager::retryConnection(uint32_t now, const char* reason) {
    (void)reason;
    ++retryCount_;
    EWFM_WIFI_LOG_DEBUG("%s %u", reason, retryCount_);
    nextRetryAt_ = now + config_.wifiRuntime.retryDelayMs;
    driver_.beginStation(credentials_);
    setState((PState)&WifiManager::Connecting, now);
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
