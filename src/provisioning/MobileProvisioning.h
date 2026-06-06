#pragma once

#include "config/DeviceConfig.h"
#include "core/Clock.h"
#include "core/StateMachine.h"
#include "provisioning/ProvisioningCoordinator.h"

#if defined(ARDUINO) && !defined(UNIT_TEST)
#include <Arduino.h>
#endif

namespace ewfm {

class MobileProvisioning : public StateMachine {
public:
    MobileProvisioning(ProvisioningCoordinator& coordinator, IClock& clock);

    void begin(const DeviceConfig& config);
    void start(uint32_t now);
    void restartBle(uint32_t now);
    void stop(uint32_t now);
    void tick(uint32_t now) override;

    bool running() const {
        return is((PState)&MobileProvisioning::Running);
    }
    bool idle() const {
        return is((PState)&MobileProvisioning::Idle);
    }
    bool timedOut() const {
        return is((PState)&MobileProvisioning::TimedOut);
    }
    bool succeeded() const {
        return is((PState)&MobileProvisioning::Succeeded);
    }
    bool restartPending() const {
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
    bool autoStartEligible() const;
    bool shouldAutoStart() const;
    void startSession(uint32_t now);
    void scheduleStart(uint32_t now);
    bool startCooldownElapsed(uint32_t now) const;
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

    ProvisioningCoordinator& coordinator_;
    IClock& clock_;
    DeviceConfig config_;
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
