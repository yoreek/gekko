#include "provisioning/MobileProvisioning.h"

#if defined(ARDUINO) && !defined(UNIT_TEST) && __has_include(<WiFiProv.h>)
#include <WiFi.h>
#include <WiFiProv.h>
#define EWFM_HAS_WIFI_PROV 1
#else
#define EWFM_HAS_WIFI_PROV 0
#endif

namespace ewfm {

#if EWFM_HAS_WIFI_PROV
namespace {
MobileProvisioning* activeProvisioning = nullptr;
}
#endif

MobileProvisioning::MobileProvisioning(ProvisioningCoordinator& coordinator, IClock& clock) : coordinator_(coordinator), clock_(clock) {}

void MobileProvisioning::begin(const DeviceConfig& config) {
    config_ = config;
    if (!config_.provisioning.mobileProvisioningEnabled) {
        state_.transitionTo(MobileProvisioningState::Disabled, clock_.millis());
    }
}

void MobileProvisioning::start() {
    if (!config_.provisioning.mobileProvisioningEnabled) {
        state_.transitionTo(MobileProvisioningState::Disabled, clock_.millis());
        return;
    }

#if EWFM_HAS_WIFI_PROV
    activeProvisioning = this;
    WiFi.onEvent([](arduino_event_t* event) {
        if (activeProvisioning == nullptr) {
            return;
        }
        switch (event->event_id) {
        case ARDUINO_EVENT_PROV_CRED_RECV:
            activeProvisioning->handleCredentials(reinterpret_cast<const char*>(event->event_info.prov_cred_recv.ssid),
                                                  reinterpret_cast<const char*>(event->event_info.prov_cred_recv.password));
            break;
        case ARDUINO_EVENT_PROV_CRED_SUCCESS:
            activeProvisioning->state_.transitionTo(MobileProvisioningState::Succeeded, activeProvisioning->clock_.millis());
            break;
        case ARDUINO_EVENT_PROV_CRED_FAIL:
            activeProvisioning->state_.transitionTo(MobileProvisioningState::Failed, activeProvisioning->clock_.millis());
            break;
        default:
            break;
        }
    });

    const char* pop = config_.provisioning.proofOfPossession.c_str();
    const char* serviceKey = config_.provisioning.serviceKey.empty() ? nullptr : config_.provisioning.serviceKey.c_str();
    const bool resetProvisioned = config_.provisioning.resetProvisionedOnStart;
    prov_scheme_t scheme = config_.provisioning.mobileBleTransport ? WIFI_PROV_SCHEME_BLE : WIFI_PROV_SCHEME_SOFTAP;
    scheme_handler_t handler = config_.provisioning.mobileBleTransport ? WIFI_PROV_SCHEME_HANDLER_FREE_BTDM : WIFI_PROV_SCHEME_HANDLER_NONE;
    WiFiProv.beginProvision(scheme, handler, WIFI_PROV_SECURITY_1, pop, config_.deviceName.c_str(), serviceKey, nullptr, resetProvisioned);
#endif
    state_.transitionTo(MobileProvisioningState::Running, clock_.millis());
}

void MobileProvisioning::stop() {
#if EWFM_HAS_WIFI_PROV
    wifi_prov_mgr_deinit();
    if (activeProvisioning == this) {
        activeProvisioning = nullptr;
    }
#endif
    state_.transitionTo(MobileProvisioningState::Idle, clock_.millis());
}

void MobileProvisioning::tick() {
    if (!state_.is(MobileProvisioningState::Running)) {
        return;
    }
    if (state_.elapsed(clock_.millis(), config_.provisioning.sessionTimeoutMs)) {
        stop();
        state_.transitionTo(MobileProvisioningState::TimedOut, clock_.millis());
    }
}

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
        state_.transitionTo(MobileProvisioningState::Failed, clock_.millis());
    }
}

} // namespace ewfm
