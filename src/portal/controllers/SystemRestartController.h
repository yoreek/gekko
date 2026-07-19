#pragma once

#include "devices/core/DeviceTypes.h"

namespace ewfm {

class DeviceRegistry;

class ISystemRestartPrecondition {
public:
    virtual ~ISystemRestartPrecondition() = default;
    virtual DeviceValidationResult flushBeforeRestart() = 0;
};

class DeviceRegistryRestartPrecondition final : public ISystemRestartPrecondition {
public:
    explicit DeviceRegistryRestartPrecondition(DeviceRegistry* registry) : registry_(registry) {}
    DeviceValidationResult flushBeforeRestart() override;

private:
    DeviceRegistry* registry_{nullptr};
};

struct SystemRestartDecision {
    DeviceValidationResult validation{};
    bool rebooting{false};

    bool ok() const {
        return validation.ok();
    }
};

class SystemRestartController {
public:
    static SystemRestartDecision requestRestart(ISystemRestartPrecondition& precondition);

    // Arms a one-shot timer that calls ESP.restart() shortly after returning, giving the caller
    // (an HTTP response, an MQTT publish) time to flush before the reboot actually happens. No-op
    // outside a real Arduino build.
    static void scheduleReboot();
};

} // namespace ewfm
