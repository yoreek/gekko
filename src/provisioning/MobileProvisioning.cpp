#include "provisioning/MobileProvisioning.h"

#include "debug/Debug.h"

#if defined(ARDUINO) && !defined(UNIT_TEST) && __has_include(<WiFiProv.h>)
#include <WiFi.h>
#include <WiFiProv.h>
#define EWFM_HAS_WIFI_PROV 1
#else
#define EWFM_HAS_WIFI_PROV 0
#endif

namespace ewfm {

#undef SM_CLASS
#define SM_CLASS MobileProvisioning

#if EWFM_HAS_WIFI_PROV
namespace {
MobileProvisioning* activeProvisioning = nullptr;
bool wifiEventRegistered = false;
} // namespace
#endif

MobileProvisioning::MobileProvisioning(ProvisioningCoordinator& coordinator, IClock& clock)
    : StateMachine((PState)&MobileProvisioning::Idle), coordinator_(coordinator), clock_(clock) {}

void MobileProvisioning::begin(const DeviceConfig& config) {
    config_ = config;
}

void MobileProvisioning::start(uint32_t now) {
    startSession(now, false);
}

void MobileProvisioning::restartBle(uint32_t now) {
    if (sessionActive_) {
        finishSession(now, "restart", (PState)&MobileProvisioning::Idle);
    }
    startSession(now, true);
}

void MobileProvisioning::startSession(uint32_t now, bool forceBleTransport) {
    if (!config_.provisioning.mobileProvisioningEnabled) {
        setState((PState)&MobileProvisioning::Disabled, now);
        return;
    }

    if (sessionActive_) {
        return;
    }

    sessionActive_ = true;
    bleTransportActive_ = forceBleTransport || config_.provisioning.mobileBleTransport;
    setState((PState)&MobileProvisioning::Running, now);

#if EWFM_HAS_WIFI_PROV
    activeProvisioning = this;
    if (!wifiEventRegistered) {
        WiFi.onEvent([](arduino_event_t* event) {
            if (activeProvisioning == nullptr) {
                return;
            }
            switch (event->event_id) {
            case ARDUINO_EVENT_PROV_CRED_RECV:
                activeProvisioning->postEvent(PendingEvent::CredentialsReceived,
                                              reinterpret_cast<const char*>(event->event_info.prov_cred_recv.ssid),
                                              reinterpret_cast<const char*>(event->event_info.prov_cred_recv.password));
                break;
            case ARDUINO_EVENT_PROV_CRED_SUCCESS:
                activeProvisioning->postEvent(PendingEvent::CredentialsSucceeded);
                break;
            case ARDUINO_EVENT_PROV_CRED_FAIL:
                activeProvisioning->postEvent(PendingEvent::CredentialsFailed);
                break;
            default:
                break;
            }
        });
        wifiEventRegistered = true;
    }

    const char* pop = config_.provisioning.proofOfPossession.c_str();
    const char* serviceKey = config_.provisioning.serviceKey.empty() ? nullptr : config_.provisioning.serviceKey.c_str();
    const bool resetProvisioned = config_.provisioning.resetProvisionedOnStart;
    prov_scheme_t scheme = bleTransportActive_ ? WIFI_PROV_SCHEME_BLE : WIFI_PROV_SCHEME_SOFTAP;
    scheme_handler_t handler = WIFI_PROV_SCHEME_HANDLER_NONE;
    if (bleTransportActive_) {
        String mac = WiFi.macAddress();
        mac.replace(":", "");
        std::string suffix = mac.c_str();
        if (suffix.empty()) {
            suffix = "esp32";
        } else if (suffix.size() > 6) {
            suffix = suffix.substr(suffix.size() - 6);
        }
        bleServiceName_ = "PROV_" + suffix;
    } else {
        bleServiceName_.clear();
    }
    const char* serviceName = bleTransportActive_ ? bleServiceName_.c_str() : config_.deviceName.c_str();
    EWFM_APP_LOG_INFO("PROV_START transport=%s", bleTransportActive_ ? "ble" : "softap");
    EWFM_PROV_LOG_INFO("PROV_START transport=%s", bleTransportActive_ ? "ble" : "softap");
    WiFiProv.beginProvision(scheme, handler, WIFI_PROV_SECURITY_1, pop, serviceName, serviceKey, nullptr, resetProvisioned);
    if (bleTransportActive_) {
        WiFiProv.printQR(serviceName, pop, "ble");
    }
#endif
}

void MobileProvisioning::stop(uint32_t now) {
    finishSession(now, "stopped", (PState)&MobileProvisioning::Idle);
}

void MobileProvisioning::finishSession(uint32_t now, const char* reason, PState terminalState) {
    if (!sessionActive_) {
        setState(terminalState, now);
        return;
    }

#if EWFM_HAS_WIFI_PROV
    EWFM_APP_LOG_INFO("PROV_END reason=%s transport=%s", reason, bleTransportActive_ ? "ble" : "softap");
    EWFM_PROV_LOG_INFO("PROV_END reason=%s transport=%s", reason, bleTransportActive_ ? "ble" : "softap");
    if (activeProvisioning == this) {
        activeProvisioning = nullptr;
    }
    wifi_prov_mgr_deinit();
#else
    (void)reason;
#endif
    sessionActive_ = false;
    bleTransportActive_ = false;
    bleServiceName_.clear();
    setState(terminalState, now);
}

void MobileProvisioning::tick(uint32_t now) {
    loop(now);
}

SM_STATE(Disabled) {}

SM_STATE(Idle) {
    if (!config_.provisioning.mobileProvisioningEnabled) {
        SM_GOTO(Disabled);
    }
}

SM_STATE(Running) {
    PendingEvent event = PendingEvent::None;
    char ssid[kMaxSsidLength + 1]{};
    char password[kMaxPasswordLength + 1]{};
    if (takePendingEvent(event, ssid, sizeof(ssid), password, sizeof(password))) {
        switch (event) {
        case PendingEvent::CredentialsReceived:
            handleCredentials(ssid, password);
            return;
        case PendingEvent::CredentialsSucceeded:
            finishSession(uptime(), "succeeded", (PState)&MobileProvisioning::Succeeded);
            return;
        case PendingEvent::CredentialsFailed:
            finishSession(uptime(), "failed", (PState)&MobileProvisioning::Failed);
            return;
        case PendingEvent::None:
            break;
        }
    }

    if (isTimeout(config_.provisioning.sessionTimeoutMs)) {
        finishSession(uptime(), "timeout", (PState)&MobileProvisioning::TimedOut);
    }
}

SM_STATE(Succeeded) {}

SM_STATE(Failed) {}

SM_STATE(TimedOut) {}

void MobileProvisioning::handleCredentials(const char* ssid, const char* password) {
    WiFiCredentials credentials;
    if (ssid != nullptr) {
        credentials.ssid = ssid;
    }
    if (password != nullptr) {
        credentials.password = password;
    }
    ProvisioningResult result = coordinator_.submitWifiCredentials(credentials);
    if (result != ProvisioningResult::Accepted) {
        finishSession(uptime(), "credentials_rejected", (PState)&MobileProvisioning::Failed);
        return;
    }
    finishSession(uptime(), "credentials_accepted", (PState)&MobileProvisioning::Succeeded);
}

void MobileProvisioning::postEvent(PendingEvent event, const char* ssid, const char* password) {
#if EWFM_HAS_WIFI_PROV
    portENTER_CRITICAL(&pendingMutex_);
    pendingEvent_ = event;
    if (ssid != nullptr) {
        strlcpy(pendingSsid_, ssid, sizeof(pendingSsid_));
    } else {
        pendingSsid_[0] = '\0';
    }
    if (password != nullptr) {
        strlcpy(pendingPassword_, password, sizeof(pendingPassword_));
    } else {
        pendingPassword_[0] = '\0';
    }
    portEXIT_CRITICAL(&pendingMutex_);
#else
    (void)event;
    (void)ssid;
    (void)password;
#endif
}

bool MobileProvisioning::takePendingEvent(PendingEvent& event, char* ssid, size_t ssidSize, char* password, size_t passwordSize) {
#if EWFM_HAS_WIFI_PROV
    portENTER_CRITICAL(&pendingMutex_);
    event = pendingEvent_;
    if (pendingEvent_ != PendingEvent::None) {
        if (ssid != nullptr && ssidSize > 0) {
            strlcpy(ssid, pendingSsid_, ssidSize);
        }
        if (password != nullptr && passwordSize > 0) {
            strlcpy(password, pendingPassword_, passwordSize);
        }
        pendingEvent_ = PendingEvent::None;
        pendingSsid_[0] = '\0';
        pendingPassword_[0] = '\0';
        portEXIT_CRITICAL(&pendingMutex_);
        return true;
    }
    portEXIT_CRITICAL(&pendingMutex_);
#else
    (void)event;
    (void)ssid;
    (void)ssidSize;
    (void)password;
    (void)passwordSize;
#endif
    return false;
}

} // namespace ewfm
