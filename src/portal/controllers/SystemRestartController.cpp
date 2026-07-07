#include "portal/controllers/SystemRestartController.h"

#include "devices/registry/DeviceRegistry.h"

#if defined(ARDUINO) && !defined(UNIT_TEST)
#include <esp_timer.h>
#endif

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

#if defined(ARDUINO) && !defined(UNIT_TEST)
namespace {
void restartTimerCallback(void*) {
    ESP.restart();
}
} // namespace
#endif

void SystemRestartController::scheduleReboot() {
#if defined(ARDUINO) && !defined(UNIT_TEST)
    static esp_timer_handle_t timerHandle = nullptr;
    if (timerHandle == nullptr) {
        const esp_timer_create_args_t args = {
            .callback = &restartTimerCallback,
            .arg = nullptr,
            .dispatch_method = ESP_TIMER_TASK,
            .name = "api-restart",
            .skip_unhandled_events = false,
        };
        (void)esp_timer_create(&args, &timerHandle);
    }
    if (timerHandle != nullptr) {
        (void)esp_timer_stop(timerHandle);
        (void)esp_timer_start_once(timerHandle, 200000);
    }
#endif
}

} // namespace ewfm
