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

namespace {
constexpr uint32_t kProvisioningRestartDelayMs = 1500;
const char* kBleServiceName = "PROV_123";
const char* kBlePop = "abcd1234";
uint8_t kBleServiceUuid[16] = {0xb4, 0xdf, 0x5a, 0x1c, 0x3f, 0x6b, 0xf4, 0xbf, 0xea, 0x4a, 0x82, 0x03, 0x04, 0x90, 0x1a, 0x02};
} // namespace

MobileProvisioning::MobileProvisioning(ProvisioningCoordinator& coordinator, IClock& clock)
    : StateMachine((PState)&MobileProvisioning::Idle), coordinator_(coordinator), clock_(clock) {}

void MobileProvisioning::begin(const DeviceConfig& config) {
    config_ = config;
}

void MobileProvisioning::start(uint32_t now) {
    startSession(now);
}

void MobileProvisioning::restartBle(uint32_t now) {
    if (sessionActive_) {
        finishSession(now, "restart", (PState)&MobileProvisioning::Idle);
    }
    if (!startCooldownElapsed(now)) {
        scheduleStart(now);
        return;
    }
    startSession(now);
}

void MobileProvisioning::startSession(uint32_t now) {
    if (!config_.provisioning.mobileProvisioningEnabled) {
        setState((PState)&MobileProvisioning::Disabled, now);
        return;
    }

    if (sessionActive_) {
        return;
    }

    if (!startCooldownElapsed(now)) {
        scheduleStart(now);
        return;
    }

    sessionActive_ = true;
    setState((PState)&MobileProvisioning::Running, now);

#if EWFM_HAS_WIFI_PROV
    activeProvisioning = this;
    if (!wifiEventRegistered) {
        WiFi.onEvent([](arduino_event_t* event) {
            if (activeProvisioning == nullptr) {
                return;
            }
            switch (event->event_id) {
            case ARDUINO_EVENT_PROV_START:
                EWFM_PROV_LOG_INFO("event PROV_START");
                break;
            case ARDUINO_EVENT_PROV_CRED_RECV:
                EWFM_PROV_LOG_INFO("event PROV_CRED_RECV ssid=%s", reinterpret_cast<const char*>(event->event_info.prov_cred_recv.ssid));
                activeProvisioning->postEvent(PendingEvent::CredentialsReceived,
                                              reinterpret_cast<const char*>(event->event_info.prov_cred_recv.ssid),
                                              reinterpret_cast<const char*>(event->event_info.prov_cred_recv.password));
                break;
            case ARDUINO_EVENT_PROV_CRED_SUCCESS:
                EWFM_PROV_LOG_INFO("event PROV_CRED_SUCCESS");
                activeProvisioning->postEvent(PendingEvent::CredentialsSucceeded);
                break;
            case ARDUINO_EVENT_PROV_CRED_FAIL:
                EWFM_PROV_LOG_WARN("event PROV_CRED_FAIL reason=%d", static_cast<int>(event->event_info.prov_fail_reason));
                activeProvisioning->postEvent(PendingEvent::CredentialsFailed);
                break;
            case ARDUINO_EVENT_PROV_END:
                EWFM_PROV_LOG_INFO("event PROV_END");
                break;
            default:
                break;
            }
        });
        wifiEventRegistered = true;
    }

    beginBleProvisioning();
#endif
}

void MobileProvisioning::scheduleStart(uint32_t now) {
    pendingStart_ = true;
    restartWaitLogged_ = false;
    setState((PState)&MobileProvisioning::RestartPending, now);
}

bool MobileProvisioning::startCooldownElapsed(uint32_t now) const {
    return nextStartAllowedAt_ == 0 || timeReached(now, nextStartAllowedAt_);
}

void MobileProvisioning::beginBleProvisioning() {
#if EWFM_HAS_WIFI_PROV
    const bool resetProvisioned = true;

    EWFM_APP_LOG_INFO("PROV_START transport=ble service=%s", kBleServiceName);
    EWFM_PROV_LOG_INFO("PROV_START transport=ble service=%s", kBleServiceName);
    WiFiProv.beginProvision(WIFI_PROV_SCHEME_BLE, WIFI_PROV_SCHEME_HANDLER_NONE, WIFI_PROV_SECURITY_1, kBlePop, kBleServiceName, nullptr,
                            kBleServiceUuid, resetProvisioned);
    WiFiProv.printQR(kBleServiceName, kBlePop, "ble");
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
    EWFM_APP_LOG_INFO("PROV_END reason=%s transport=ble", reason);
    EWFM_PROV_LOG_INFO("PROV_END reason=%s transport=ble", reason);
    wifi_prov_mgr_deinit();
    if (activeProvisioning == this) {
        activeProvisioning = nullptr;
    }
#else
    (void)reason;
#endif
    nextStartAllowedAt_ = now + kProvisioningRestartDelayMs;
    restartWaitLogged_ = false;
    sessionActive_ = false;
    setState(terminalState, now);
}

void MobileProvisioning::tick(uint32_t now) {
    StateMachine::tick(now);
    runLifecyclePolicy(now);
}

void MobileProvisioning::runLifecyclePolicy(uint32_t now) {
    if (!config_.provisioning.mobileProvisioningEnabled) {
        return;
    }

    if (coordinator_.takeMobileProvisioningReentryRequest()) {
        handleReentryRequest(now);
        return;
    }

    if (restartPending()) {
        return;
    }

    if (!shouldAutoStart()) {
        return;
    }

    EWFM_APP_LOG_DEBUG("starting mobile provisioning");
    startSession(now);
}

void MobileProvisioning::handleReentryRequest(uint32_t now) {
    EWFM_APP_LOG_INFO("PROV_REENTER handling");
    EWFM_APP_LOG_INFO("PROV_REENTER clearing wifi credentials and starting BLE provisioning");
    coordinator_.resetWifiCredentialsAt(now);
    restartBle(now);
}

bool MobileProvisioning::autoStartEligible() const {
    return idle() || succeeded() || timedOut();
}

bool MobileProvisioning::shouldAutoStart() const {
    if (running() || !autoStartEligible()) {
        return false;
    }

    const bool missingCredentials = !coordinator_.hasWifiCredentials();
    if (missingCredentials && (idle() || timedOut())) {
        return true;
    }

    return coordinator_.wifiProvisioningFallback();
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
            handleCredentials(ssid, password, uptime());
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

SM_STATE(RestartPending) {
    if (!config_.provisioning.mobileProvisioningEnabled) {
        pendingStart_ = false;
        SM_GOTO(Disabled);
    }

    if (!pendingStart_) {
        SM_GOTO(Idle);
    }

    if (!startCooldownElapsed(uptime())) {
        if (!restartWaitLogged_) {
            EWFM_APP_LOG_INFO("PROV_RESTART waiting for provisioning manager cooldown");
            EWFM_PROV_LOG_INFO("PROV_RESTART waiting for provisioning manager cooldown");
            restartWaitLogged_ = true;
        }
        return;
    }

    pendingStart_ = false;
    startSession(uptime());
}

void MobileProvisioning::handleCredentials(const char* ssid, const char* password, uint32_t now) {
    WiFiCredentials credentials;
    if (ssid != nullptr) {
        credentials.ssid = ssid;
    }
    if (password != nullptr) {
        credentials.password = password;
    }
    ProvisioningResult result = coordinator_.submitWifiCredentialsAt(credentials, now);
    if (result != ProvisioningResult::Accepted) {
        finishSession(now, "credentials_rejected", (PState)&MobileProvisioning::Failed);
        return;
    }
    finishSession(now, "credentials_accepted", (PState)&MobileProvisioning::Succeeded);
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
