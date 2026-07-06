#include "portal/controllers/MqttController.h"
#include "portal/controllers/SystemController.h"
#include "portal/controllers/SystemRestartController.h"

#include <unity.h>

using namespace ewfm;

namespace {

class StubRestartPrecondition final : public ISystemRestartPrecondition {
public:
    explicit StubRestartPrecondition(DeviceValidationResult result) : result_(result) {}

    DeviceValidationResult flushBeforeRestart() override {
        ++calls_;
        return result_;
    }

    uint16_t calls() const {
        return calls_;
    }

private:
    DeviceValidationResult result_{};
    uint16_t calls_{0};
};

} // namespace

void test_restart_controller_requests_reboot_when_flush_succeeds() {
    StubRestartPrecondition precondition(DeviceValidationResult{});

    const SystemRestartDecision decision = SystemRestartController::requestRestart(precondition);

    TEST_ASSERT_TRUE(decision.ok());
    TEST_ASSERT_TRUE(decision.rebooting);
    TEST_ASSERT_EQUAL_UINT16(1, precondition.calls());
}

void test_restart_controller_rejects_reboot_when_flush_fails() {
    StubRestartPrecondition precondition(DeviceValidationResult{DeviceError::StorageError, "flush failed"});

    const SystemRestartDecision decision = SystemRestartController::requestRestart(precondition);

    TEST_ASSERT_FALSE(decision.ok());
    TEST_ASSERT_FALSE(decision.rebooting);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(DeviceError::StorageError), static_cast<int>(decision.validation.error));
    TEST_ASSERT_EQUAL_STRING("flush failed", decision.validation.message);
    TEST_ASSERT_EQUAL_UINT16(1, precondition.calls());
}

void test_restart_route_build_flag_state_matches_compilation() {
#if defined(WITH_SYSTEM_RESTART_API)
    TEST_ASSERT_TRUE(SystemController::restartApiEnabledForBuild());
#else
    TEST_ASSERT_FALSE(SystemController::restartApiEnabledForBuild());
#endif
}

void test_mqtt_route_build_flag_state_matches_compilation() {
#if defined(WITH_HOME_ASSISTANT)
    TEST_ASSERT_TRUE(MqttController::compiledIn());
#else
    TEST_ASSERT_FALSE(MqttController::compiledIn());
#endif
}
