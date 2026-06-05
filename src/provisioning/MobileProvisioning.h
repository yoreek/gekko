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
    void stop(uint32_t now);
    void tick(uint32_t now);

    bool running() const {
        return is((PState)&MobileProvisioning::Running);
    }
    bool timedOut() const {
        return is((PState)&MobileProvisioning::TimedOut);
    }

private:
    enum class PendingEvent {
        None,
        CredentialsReceived,
        CredentialsSucceeded,
        CredentialsFailed,
    };

    void handleCredentials(const char* ssid, const char* password);
    void postEvent(PendingEvent event, const char* ssid = nullptr, const char* password = nullptr);
    bool takePendingEvent(PendingEvent& event, char* ssid, size_t ssidSize, char* password, size_t passwordSize);
    void Disabled();
    void Idle();
    void Running();
    void Succeeded();
    void Failed();
    void TimedOut();

    ProvisioningCoordinator& coordinator_;
    IClock& clock_;
    DeviceConfig config_;

#if defined(ARDUINO) && !defined(UNIT_TEST)
    char pendingSsid_[kMaxSsidLength + 1]{};
    char pendingPassword_[kMaxPasswordLength + 1]{};
    PendingEvent pendingEvent_{PendingEvent::None};
    portMUX_TYPE pendingMutex_ = portMUX_INITIALIZER_UNLOCKED;
#endif
};

} // namespace ewfm
