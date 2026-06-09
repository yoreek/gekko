#include "portal/routes/SystemRestartController.h"

#include "devices/registry/DeviceRegistry.h"

namespace ewfm {

DeviceValidationResult DeviceRegistryRestartPrecondition::flushBeforeRestart() {
    if (registry_ == nullptr) {
        return {};
    }
    return registry_->flushNow();
}

SystemRestartDecision SystemRestartController::requestRestart(ISystemRestartPrecondition& precondition) {
    SystemRestartDecision decision{};
    decision.validation = precondition.flushBeforeRestart();
    decision.rebooting = decision.validation.ok();
    return decision;
}

} // namespace ewfm
