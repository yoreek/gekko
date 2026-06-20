#include "devices/core/DeviceBaseConfig.h"
#include "devices/core/DeviceRuntimeBase.h"

#include <cstdio>
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

void test_device_runtime_base_dependency_wiring() {
    TestRuntime runtime;
    StatusRuntime dependency;
    StatusRuntime dependent;

    DeviceRegistryEntry record{};
    record.header.recordVersion = kDeviceRecordHeaderVersion;
    record.header.deviceId = 42;
    record.header.typeId = 7;
    record.header.configVersion = 1;
    record.header.configRevision = 1;
    record.depCount = 1;
    record.deps[0] = {DeviceDependencyRole::OneWireBus, 99};

    DeviceBaseConfigV1 base{};
    base.enabled = 1;
    std::snprintf(base.name, sizeof(base.name), "%s", "runtime");
    DeviceConfigBlob configBlob{};
    TEST_ASSERT_TRUE(configBlob.setSize(deviceBaseConfigSize(base)));
    TEST_ASSERT_TRUE(writeDeviceBaseConfig(configBlob, base));

    runtime.bindDeviceIdentity(record, configBlob);
    runtime.begin(0);
    runtime.setDependencyRuntime(DeviceDependencyRole::OneWireBus, &dependency);
    TEST_ASSERT_EQUAL_PTR(&dependency, runtime.dependencyRuntime(DeviceDependencyRole::OneWireBus));

    runtime.attachDependentRuntime(&dependent);
    runtime.attachDependentRuntime(&dependent);
    TEST_ASSERT_EQUAL_UINT32(1, runtime.dependentRuntimes().size());
    TEST_ASSERT_EQUAL_PTR(&dependent, runtime.dependentRuntimes()[0]);

    runtime.detachDependentRuntime(&dependent);
    TEST_ASSERT_TRUE(runtime.dependentRuntimes().empty());
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
    RUN_TEST(test_device_runtime_base_dependency_wiring);
    RUN_TEST(test_device_runtime_base_request_status_defaults);
    return UNITY_END();
}
