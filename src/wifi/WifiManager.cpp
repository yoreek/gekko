#include "wifi/WifiManager.h"

#include "debug/Debug.h"

namespace ewfm {

#undef SM_CLASS
#define SM_CLASS WifiManager

WifiManager::WifiManager(IWifiDriver& driver) : StateMachine((PState)&WifiManager::Idle), driver_(driver) {}

void WifiManager::begin(const DeviceConfig& config) {
    driver_.begin();
    config_ = config;
    credentials_ = config.wifi;
    retryCount_ = 0;
    stationIpLogged_ = false;
    stationConnectRequested_ = credentials_.hasCredentials();
}

void WifiManager::updateCredentials(const WiFiCredentials& credentials) {
    credentials_ = credentials;
    retryCount_ = 0;
    stationIpLogged_ = false;
    stationConnectRequested_ = credentials_.hasCredentials();
}

void WifiManager::clearCredentials() {
    credentials_ = {};
    retryCount_ = 0;
    stationIpLogged_ = false;
    stationConnectRequested_ = false;
}

SM_STATE(Idle) {
    if (stationConnectRequested_ && credentials_.hasCredentials()) {
        EWFM_WIFI_LOG_INFO("stored credentials found, connecting");
        SM_GOTO(Connecting);
    }

    if (config_.wifiRuntime.setupApEnabled) {
        EWFM_WIFI_LOG_INFO("no station credentials, starting setup AP");
        if (!driver_.setupApActive()) {
            if (driver_.startSetupAp(setupApSsid(), config_.provisioning.setupApPassword)) {
                EWFM_WIFI_LOG_INFO("setup AP started");
                if (!driver_.prepareProvisioningScan()) {
                    EWFM_WIFI_LOG_WARN("provisioning scan preparation failed");
                }
            } else {
                EWFM_WIFI_LOG_WARN("setup AP start failed");
            }
        }
        SM_GOTO(SetupAp);
    }
}

SM_STATE(Connecting) {
    if (isStateUpdated()) {
        if (!credentials_.hasCredentials()) {
            EWFM_WIFI_LOG_WARN("station connection requested without credentials");
            SM_GOTO(Idle);
        }

        driver_.beginStation(credentials_);
        EWFM_WIFI_LOG_INFO("station connection started for ssid=%s", credentials_.ssid.c_str());
        stationConnectRequested_ = false;
        SM_GOTO(CheckConnection);
    }
}

SM_STATE(CheckConnection) {
    switch (driver_.status()) {
    case WifiDriverStatus::Connected:
        EWFM_WIFI_LOG_INFO("station connected");
        stationIpLogged_ = false;
        stationConnectRequested_ = false;
        SM_GOTO(Connected);
    case WifiDriverStatus::Failed:
    case WifiDriverStatus::Disconnected:
        stationIpLogged_ = false;
        ++retryCount_;
        if (retriesExhausted()) {
            if (config_.wifiRuntime.setupApEnabled) {
                EWFM_WIFI_LOG_WARN("station connection failed, starting setup AP");
                if (!driver_.setupApActive()) {
                    if (driver_.startSetupAp(setupApSsid(), config_.provisioning.setupApPassword)) {
                        EWFM_WIFI_LOG_INFO("setup AP started");
                        if (!driver_.prepareProvisioningScan()) {
                            EWFM_WIFI_LOG_WARN("provisioning scan preparation failed");
                        }
                    } else {
                        EWFM_WIFI_LOG_WARN("setup AP start failed");
                    }
                }
                stationConnectRequested_ = false;
                SM_GOTO(SetupAp);
            }
            SM_GOTO(Idle);
        }

        EWFM_WIFI_LOG_DEBUG("connection retry %u", retryCount_);
        if (config_.wifiRuntime.retryDelayMs == 0) {
            SM_GOTO(Connecting);
        }
        SM_GOTO(RetryDelay);
    case WifiDriverStatus::Connecting:
    case WifiDriverStatus::Idle:
        if (isTimeout(config_.wifiRuntime.connectTimeoutMs)) {
            stationIpLogged_ = false;
            ++retryCount_;
            if (retriesExhausted()) {
                if (config_.wifiRuntime.setupApEnabled) {
                    EWFM_WIFI_LOG_WARN("station connection timeout, starting setup AP");
                    if (!driver_.setupApActive()) {
                        if (driver_.startSetupAp(setupApSsid(), config_.provisioning.setupApPassword)) {
                            EWFM_WIFI_LOG_INFO("setup AP started");
                            if (!driver_.prepareProvisioningScan()) {
                                EWFM_WIFI_LOG_WARN("provisioning scan preparation failed");
                            }
                        } else {
                            EWFM_WIFI_LOG_WARN("setup AP start failed");
                        }
                    }
                    stationConnectRequested_ = false;
                    SM_GOTO(SetupAp);
                }
                SM_GOTO(Idle);
            }

            EWFM_WIFI_LOG_DEBUG("connection timeout retry %u", retryCount_);
            if (config_.wifiRuntime.retryDelayMs == 0) {
                SM_GOTO(Connecting);
            }
            SM_GOTO(RetryDelay);
        }
        break;
    }
}

SM_STATE(RetryDelay) {
    if (isTimeout(config_.wifiRuntime.retryDelayMs)) {
        SM_GOTO(Connecting);
    }
}

SM_STATE(Connected) {
    if (!stationIpLogged_) {
        const std::string ip = driver_.stationIp();
        if (!ip.empty()) {
            EWFM_WIFI_LOG_INFO("station ip=%s", ip.c_str());
            stationIpLogged_ = true;
        }
    }

    if (!credentials_.hasCredentials()) {
        stationIpLogged_ = false;
        EWFM_WIFI_LOG_WARN("station credentials cleared, falling back");
        if (config_.wifiRuntime.setupApEnabled) {
            if (!driver_.setupApActive()) {
                if (driver_.startSetupAp(setupApSsid(), config_.provisioning.setupApPassword)) {
                    EWFM_WIFI_LOG_INFO("setup AP started");
                    if (!driver_.prepareProvisioningScan()) {
                        EWFM_WIFI_LOG_WARN("provisioning scan preparation failed");
                    }
                } else {
                    EWFM_WIFI_LOG_WARN("setup AP start failed");
                }
            }
            stationConnectRequested_ = false;
            SM_GOTO(SetupAp);
        }
        stationConnectRequested_ = false;
        SM_GOTO(Idle);
    }

    switch (driver_.status()) {
    case WifiDriverStatus::Connected:
        if (isTimeout(config_.wifiRuntime.connectTimeoutMs)) {
            SM_GOTO(CheckConnection);
        }
        break;
    case WifiDriverStatus::Failed:
    case WifiDriverStatus::Disconnected:
        stationIpLogged_ = false;
        EWFM_WIFI_LOG_WARN("station disconnected, reconnecting");
        retryCount_ = 0;
        if (credentials_.hasCredentials()) {
            stationConnectRequested_ = true;
            SM_GOTO(Connecting);
        }
        if (config_.wifiRuntime.setupApEnabled) {
            if (!driver_.setupApActive()) {
                if (driver_.startSetupAp(setupApSsid(), config_.provisioning.setupApPassword)) {
                    EWFM_WIFI_LOG_INFO("setup AP started");
                    if (!driver_.prepareProvisioningScan()) {
                        EWFM_WIFI_LOG_WARN("provisioning scan preparation failed");
                    }
                } else {
                    EWFM_WIFI_LOG_WARN("setup AP start failed");
                }
            }
            stationConnectRequested_ = false;
            SM_GOTO(SetupAp);
        }
        stationConnectRequested_ = false;
        SM_GOTO(Idle);
    case WifiDriverStatus::Connecting:
    case WifiDriverStatus::Idle:
        if (isTimeout(config_.wifiRuntime.connectTimeoutMs)) {
            SM_GOTO(CheckConnection);
        }
        break;
    }
}

SM_STATE(SetupAp) {
    if (isStateUpdated() && !driver_.setupApActive()) {
        if (driver_.startSetupAp(setupApSsid(), config_.provisioning.setupApPassword)) {
            EWFM_WIFI_LOG_INFO("setup AP started");
            if (!driver_.prepareProvisioningScan()) {
                EWFM_WIFI_LOG_WARN("provisioning scan preparation failed");
            }
        } else {
            EWFM_WIFI_LOG_WARN("setup AP start failed");
        }
    }

    if (stationConnectRequested_ && credentials_.hasCredentials()) {
        SM_GOTO(Connecting);
    }

    switch (driver_.status()) {
    case WifiDriverStatus::Connected:
        EWFM_WIFI_LOG_INFO("station connected");
        stationIpLogged_ = false;
        SM_GOTO(Connected);
    case WifiDriverStatus::Failed:
    case WifiDriverStatus::Disconnected:
    case WifiDriverStatus::Connecting:
    case WifiDriverStatus::Idle:
        break;
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
