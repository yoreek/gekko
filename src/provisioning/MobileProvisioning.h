#pragma once

#include "config/DeviceConfig.h"
#include "core/Clock.h"
#include "core/StateMachine.h"
#include "provisioning/ProvisioningCoordinator.h"
#include "wifi/WifiManager.h"

#if defined(ARDUINO) && !defined(UNIT_TEST)
#include <Arduino.h>
#endif

#if defined(ARDUINO) && !defined(UNIT_TEST) && __has_include(<WiFiProv.h>)
#include <WiFi.h>
#include <WiFiProv.h>
#define EWFM_HAS_WIFI_PROV 1
#else
#define EWFM_HAS_WIFI_PROV 0
#endif

namespace ewfm {

class MobileProvisioning final : public StateMachine {
public:
    static constexpr uint32_t kProvisioningRestartDelayMs = 1500;
    static constexpr char kBlePop[] = "abcd1234";
    static constexpr char kBleServiceNamePrefix[] = "PROV_";
    static constexpr size_t kBleServiceNameSuffixLength = 6;
    static constexpr size_t kMaxBleServiceNameLength = sizeof(kBleServiceNamePrefix) - 1 + kBleServiceNameSuffixLength;

#if EWFM_HAS_WIFI_PROV
    static MobileProvisioning* activeProvisioning;
    static bool wifiEventRegistered;
#endif

    MobileProvisioning(ProvisioningCoordinator& coordinator, WifiManager& wifiManager, IClock& clock);

    void begin(const DeviceConfig& config);
    void start(uint32_t now);
    void restartBle(uint32_t now);
    void stop(uint32_t now);
    void tick(uint32_t now) override;

    [[nodiscard]] bool running() const {
        return is((PState)&MobileProvisioning::Running);
    }
    [[nodiscard]] bool idle() const {
        return is((PState)&MobileProvisioning::Idle);
    }
    [[nodiscard]] bool timedOut() const {
        return is((PState)&MobileProvisioning::TimedOut);
    }
    [[nodiscard]] bool succeeded() const {
        return is((PState)&MobileProvisioning::Succeeded);
    }
    [[nodiscard]] bool restartPending() const {
        return is((PState)&MobileProvisioning::RestartPending);
    }

private:
    enum class PendingEvent {
        None,
        CredentialsReceived,
        CredentialsSucceeded,
        CredentialsFailed,
    };

    void handleCredentials(const char* ssid, const char* password, uint32_t now);
    void runLifecyclePolicy(uint32_t now);
    void handleReentryRequest(uint32_t now);
    [[nodiscard]] bool autoStartEligible() const;
    [[nodiscard]] bool shouldAutoStart() const;
    void startSession(uint32_t now);
    void scheduleStart(uint32_t now);
    [[nodiscard]] bool startCooldownElapsed(uint32_t now) const;
    void finishSession(uint32_t now, const char* reason, PState terminalState);
    void beginBleProvisioning();
    void postEvent(PendingEvent event, const char* ssid = nullptr, const char* password = nullptr);
    bool takePendingEvent(PendingEvent& event, char* ssid, size_t ssidSize, char* password, size_t passwordSize);
    void Disabled();
    void Idle();
    void Running();
    void Succeeded();
    void Failed();
    void TimedOut();
    void RestartPending();
    void initBleServiceName();

    ProvisioningCoordinator& coordinator_;
    WifiManager& wifiManager_;
    IClock& clock_;
    DeviceConfig config_;
    char bleServiceName_[kMaxBleServiceNameLength + 1]{};
    bool sessionActive_{false};
    bool pendingStart_{false};
    bool restartWaitLogged_{false};
    uint32_t nextStartAllowedAt_{0};

#if defined(ARDUINO) && !defined(UNIT_TEST)
    char pendingSsid_[kMaxSsidLength + 1]{};
    char pendingPassword_[kMaxPasswordLength + 1]{};
    PendingEvent pendingEvent_{PendingEvent::None};
    portMUX_TYPE pendingMutex_ = portMUX_INITIALIZER_UNLOCKED;
#endif
};

} // namespace ewfm
