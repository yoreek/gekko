#pragma once

#include "config/DeviceConfig.h"
#include "core/Clock.h"
#include "core/StateMachine.h"
#include "provisioning/ProvisioningCoordinator.h"

#include <string>

namespace ewfm {

enum class MobileProvisioningState {
    Disabled,
    Idle,
    Running,
    Succeeded,
    Failed,
    TimedOut,
};

class MobileProvisioning {
public:
    MobileProvisioning(ProvisioningCoordinator& coordinator, IClock& clock);

    void begin(const DeviceConfig& config);
    void start();
    void stop();
    void tick();

    MobileProvisioningState state() const {
        return state_.state();
    }

private:
    void handleCredentials(const char* ssid, const char* password);

    ProvisioningCoordinator& coordinator_;
    IClock& clock_;
    DeviceConfig config_;
    StateMachine<MobileProvisioningState> state_{MobileProvisioningState::Idle};
};

} // namespace ewfm
