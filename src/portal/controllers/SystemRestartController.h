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
};

} // namespace ewfm
