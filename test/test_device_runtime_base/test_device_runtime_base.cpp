#include "devices/core/DeviceRuntimeBase.h"

#include <unity.h>

using namespace ewfm;

namespace {

#undef SM_CLASS
#define SM_CLASS TestRuntime
class TestRuntime final : public DeviceRuntimeBase {
public:
    TestRuntime() : DeviceRuntimeBase((PState)&TestRuntime::Idle) {}

    State Idle();

    uint32_t lastNow{0};
    uint8_t tickCount{0};
};

SM_STATE(TestRuntime::Idle) {
    ++tickCount;
    lastNow = uptime();
    if (startRequested()) {
        clearStartRequested();
        setStatus(DeviceStatus::Ready);
    }
}
#undef SM_CLASS

class StatusRuntime final : public IDeviceRuntime {
public:
    void begin(uint32_t now) override {
        (void)now;
    }
    void tickFastLoop(uint32_t now) override {
        (void)now;
    }
    void tick100ms(uint32_t now) override {
        (void)now;
    }
    void tick1s(uint32_t now) override {
        (void)now;
    }
    void requestReconfigure() override {}
    void requestDisable() override {}
    void requestDelete() override {}
    DeviceStatus status() const override {
        return statusValue;
    }
    bool handleCommand(const DeviceCommand& command) override {
        (void)command;
        return false;
    }

    DeviceStatus statusValue{DeviceStatus::Unknown};
};

} // namespace

void test_device_runtime_base_begin_and_cadence_tick_state_machine() {
    TestRuntime runtime;

    runtime.begin(10);
    TEST_ASSERT_EQUAL_UINT8(1, runtime.tickCount);
    TEST_ASSERT_EQUAL_UINT32(10, runtime.lastNow);
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceStatus::Ready), static_cast<int>(runtime.status()));

    runtime.tickFastLoop(11);
    runtime.tick100ms(12);
    runtime.tick1s(13);
    TEST_ASSERT_EQUAL_UINT8(4, runtime.tickCount);
    TEST_ASSERT_EQUAL_UINT32(13, runtime.lastNow);
}

void test_device_runtime_base_parent_and_child_wiring() {
    TestRuntime runtime;
    StatusRuntime parent;
    StatusRuntime child;

    runtime.setParentRuntime(&parent);
    TEST_ASSERT_EQUAL_PTR(&parent, runtime.parentRuntime());

    runtime.attachChildRuntime(&child);
    runtime.attachChildRuntime(&child);
    TEST_ASSERT_EQUAL_UINT32(1, runtime.childRuntimes().size());
    TEST_ASSERT_EQUAL_PTR(&child, runtime.childRuntimes()[0]);

    runtime.detachChildRuntime(&child);
    TEST_ASSERT_TRUE(runtime.childRuntimes().empty());
}

void test_device_runtime_base_request_status_defaults() {
    TestRuntime runtime;

    runtime.requestReconfigure();
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceStatus::Reconfiguring), static_cast<int>(runtime.status()));

    runtime.requestDisable();
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceStatus::Disabled), static_cast<int>(runtime.status()));

    runtime.requestDelete();
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceStatus::Deleting), static_cast<int>(runtime.status()));
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_device_runtime_base_begin_and_cadence_tick_state_machine);
    RUN_TEST(test_device_runtime_base_parent_and_child_wiring);
    RUN_TEST(test_device_runtime_base_request_status_defaults);
    return UNITY_END();
}
