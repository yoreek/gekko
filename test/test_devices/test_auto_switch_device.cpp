#include "config/MemoryConfigStorage.h"
#include "devices/core/DeviceIdGenerator.h"
#include "devices/core/DeviceTypes.h"
#include "devices/registry/DeviceRegistry.h"
#include "devices/registry/DeviceRetainedDataStore.h"
#include "devices/switch/SwitchOutputState.h"
#include "devices/switch/auto/AutoSwitchDevice.h"
#include "devices/switch/auto/AutoSwitchDeviceConfig.h"
#include "integrations/rest/auto_switch/AutoSwitchDeviceApiAdapter.h"

#include <algorithm>
#include <array>
#include <cstdio>
#include <unity.h>

using namespace ewfm;

namespace {

class FakeSwitch final : public DeviceRuntimeBase, public ISwitchOutputRuntime, public IStatusRuntime {
public:
    FakeSwitch() : DeviceRuntimeBase((PState)&FakeSwitch::Idle) {
        status_ = DeviceStatus::Ready;
    }

    void begin(uint32_t) override {
        status_ = DeviceStatus::Ready;
    }
    void tickFastLoop(uint32_t) override {}
    void tick100ms(uint32_t) override {}
    void tick1s(uint32_t) override {}
    bool handleCommand(const DeviceCommand&) override {
        return false;
    }
    const ISwitchOutputRuntime* switchOutputRuntime() const override {
        return this;
    }
    const IStatusRuntime* statusRuntime() const override {
        return this;
    }
    // IStatusRuntime: lets a FakeSwitch double as a Condition-role dependency source, mirroring
    // SwitchDeviceBase::isActive() in production.
    bool isActive() const override {
        return state_;
    }
    StateType currentOutputState() const override {
        return state_;
    }
    bool requestOutputState(StateType state, uint32_t now) override {
        (void)now;
        ++requestCount;
        if (!requestOk) {
            return false;
        }
        state_ = state;
        return true;
    }

    StateType state_{kSwitchOutputOff};
    uint32_t requestCount{0U};
    bool requestOk{true};

private:
    State Idle() {
        status_ = DeviceStatus::Ready;
    }
};

class FakeSchedule final : public DeviceRuntimeBase, public IScheduleRuntime, public IStatusRuntime {
public:
    FakeSchedule() : DeviceRuntimeBase((PState)&FakeSchedule::Idle) {
        status_ = DeviceStatus::Ready;
    }

    void begin(uint32_t) override {
        status_ = DeviceStatus::Ready;
    }
    void tickFastLoop(uint32_t) override {}
    void tick100ms(uint32_t) override {}
    void tick1s(uint32_t) override {}
    bool handleCommand(const DeviceCommand&) override {
        return false;
    }
    const IScheduleRuntime* scheduleRuntime() const override {
        return this;
    }
    const IStatusRuntime* statusRuntime() const override {
        return this;
    }
    // IScheduleRuntime, IStatusRuntime -- identical `bool isActive() const` signature, one override
    // satisfies both, mirroring the real ScheduleDevice.
    bool isActive() const override {
        return active_;
    }
    bool timeValid() const override {
        return true;
    }

    bool active_{false};

private:
    State Idle() {
        status_ = DeviceStatus::Ready;
    }
};

AutoSwitchDeviceConfigV1 makeAutoSwitchConfig() {
    AutoSwitchDeviceConfigV1 config{};
    config.enabled = true;
    std::snprintf(config.name, sizeof(config.name), "%s", "auto_switch");
    config.pauseDurationSeconds = 100U;
    return config;
}

BoundedBlob<kMaxDeviceConfigBytes> encodeAutoSwitchPayload(const AutoSwitchDeviceConfigV1& config) {
    BoundedBlob<kMaxDeviceConfigBytes> payload{};
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    TEST_ASSERT_TRUE(encodeFixedConfigBlob(AutoSwitchDeviceConfigV1::kMagic, config, buffer, autoSwitchDeviceConfigSize(config)));
    TEST_ASSERT_TRUE(payload.assign(buffer, autoSwitchDeviceConfigSize(config)));
    return payload;
}

struct ConditionDep {
    DeviceId deviceId;
    bool invert{false};
};

// deps[0] is always the required target switch; deps[1..] are Condition-role links, one per entry
// of `conditions`, in order - so setDependencyRuntimeAt(1 + i, ...) wires the i-th condition.
void bindAutoSwitchIdentity(AutoSwitchDevice& device, DeviceId autoSwitchId, DeviceId switchId,
                            std::initializer_list<ConditionDep> conditions = {}) {
    const BoundedBlob<kMaxDeviceConfigBytes> configBlob = encodeAutoSwitchPayload(device.config());
    DeviceRegistryEntry record{};
    record.header.deviceId = autoSwitchId;
    record.header.typeId = kAutoSwitchDeviceTypeId;
    record.header.configVersion = kAutoSwitchDeviceConfigVersion;
    record.header.configRevision = 1U;
    record.header.payloadLength = static_cast<uint32_t>(configBlob.size());
    record.deps[0] = DeviceDependencyLink{DeviceRole::Switch, switchId, false};
    uint8_t index = 1U;
    for (const ConditionDep& condition : conditions) {
        record.deps[index++] = DeviceDependencyLink{DeviceRole::Condition, condition.deviceId, condition.invert};
    }
    record.depCount = index;
    record.status = DeviceStatus::Ready;
    device.bindDeviceIdentity(record, configBlob);
}

void startAutoSwitch(AutoSwitchDevice& device, uint32_t now = 10) {
    device.begin(now);
    device.tick1s(now + 1U);
}

} // namespace

void test_auto_switch_descriptor_and_api_adapter_are_registered() {
    const DeviceTypeDescriptor descriptor = AutoSwitchDevice::descriptor();
    TEST_ASSERT_EQUAL_STRING("AutoSwitchDevice", descriptor.name);
    TEST_ASSERT_TRUE(descriptor.providedRoles.contains(DeviceRole::Switch));
    TEST_ASSERT_TRUE(descriptor.providedRoles.contains(DeviceRole::Condition));

    const auto switchRequirement =
        std::find_if(descriptor.dependencyRequirements.begin(), descriptor.dependencyRequirements.end(),
                     [](const DeviceDependencyRequirement& requirement) { return requirement.role == DeviceRole::Switch; });
    TEST_ASSERT_TRUE(switchRequirement != descriptor.dependencyRequirements.end());
    TEST_ASSERT_TRUE(switchRequirement->required);

    const auto conditionRequirement =
        std::find_if(descriptor.dependencyRequirements.begin(), descriptor.dependencyRequirements.end(),
                     [](const DeviceDependencyRequirement& requirement) { return requirement.role == DeviceRole::Condition; });
    TEST_ASSERT_TRUE(conditionRequirement != descriptor.dependencyRequirements.end());
    TEST_ASSERT_FALSE(conditionRequirement->required);

    DeviceApiAdapterRegistry adapters = DeviceApiAdapterRegistry::withDefaults();
    const IDeviceApiAdapter* adapter = adapters.find(kAutoSwitchDeviceTypeId);
    TEST_ASSERT_NOT_NULL(adapter);
    TEST_ASSERT_EQUAL_STRING("auto_switch", adapter->typeName());
}

void test_auto_switch_dependency_blocked_without_target_switch() {
    AutoSwitchDevice device(makeAutoSwitchConfig());
    FakeSchedule schedule;
    bindAutoSwitchIdentity(device, 1U, 2U, {{3U}});
    device.setDependencyRuntime(DeviceRole::Condition, &schedule);
    startAutoSwitch(device);
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceStatus::DependencyBlocked), static_cast<int>(device.status()));
}

void test_auto_switch_auto_mode_follows_schedule() {
    AutoSwitchDevice device(makeAutoSwitchConfig());
    FakeSwitch target;
    FakeSchedule schedule;
    bindAutoSwitchIdentity(device, 10U, 11U, {{12U}});
    device.setDependencyRuntime(DeviceRole::Switch, &target);
    device.setDependencyRuntime(DeviceRole::Condition, &schedule);
    startAutoSwitch(device, 100);
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceStatus::Ready), static_cast<int>(device.status()));
    TEST_ASSERT_TRUE(device.mode() == AutoSwitchMode::Auto);
    TEST_ASSERT_TRUE(target.state_ == kSwitchOutputOff);

    schedule.active_ = true;
    device.tick1s(101);
    TEST_ASSERT_TRUE(target.state_ == kSwitchOutputOn);

    schedule.active_ = false;
    device.tick1s(102);
    TEST_ASSERT_TRUE(target.state_ == kSwitchOutputOff);
}

void test_auto_switch_auto_mode_with_no_conditions_defaults_off() {
    AutoSwitchDevice device(makeAutoSwitchConfig());
    FakeSwitch target;
    bindAutoSwitchIdentity(device, 20U, 21U);
    device.setDependencyRuntime(DeviceRole::Switch, &target);
    startAutoSwitch(device, 100);
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceStatus::Ready), static_cast<int>(device.status()));

    target.state_ = kSwitchOutputOn;
    device.tick1s(101);
    TEST_ASSERT_TRUE(target.state_ == kSwitchOutputOff);
}

void test_auto_switch_manual_override_forces_state_then_mode_auto_command_resumes_schedule() {
    AutoSwitchDevice device(makeAutoSwitchConfig());
    FakeSwitch target;
    FakeSchedule schedule;
    bindAutoSwitchIdentity(device, 30U, 31U, {{32U}});
    device.setDependencyRuntime(DeviceRole::Switch, &target);
    device.setDependencyRuntime(DeviceRole::Condition, &schedule);
    startAutoSwitch(device, 100);

    schedule.active_ = false;
    TEST_ASSERT_TRUE(device.requestOutputState(kSwitchOutputOn, 101));
    TEST_ASSERT_TRUE(device.mode() == AutoSwitchMode::On);
    TEST_ASSERT_TRUE(target.state_ == kSwitchOutputOn);

    // Auto tick must not override the manual On mode while schedule is inactive.
    device.tick1s(102);
    TEST_ASSERT_TRUE(target.state_ == kSwitchOutputOn);

    DeviceCommand resumeCommand{DeviceCommandType::Custom, device.deviceId(), "auto"};
    TEST_ASSERT_TRUE(device.handleCommand(resumeCommand));
    TEST_ASSERT_TRUE(device.mode() == AutoSwitchMode::Auto);
    device.tick1s(103);
    TEST_ASSERT_TRUE(target.state_ == kSwitchOutputOff);
}

void test_auto_switch_entering_auto_forces_off_before_next_tick_follows_active_schedule() {
    AutoSwitchDevice device(makeAutoSwitchConfig());
    FakeSwitch target;
    FakeSchedule schedule;
    bindAutoSwitchIdentity(device, 60U, 61U, {{62U}});
    device.setDependencyRuntime(DeviceRole::Switch, &target);
    device.setDependencyRuntime(DeviceRole::Condition, &schedule);
    startAutoSwitch(device, 200);

    schedule.active_ = false;
    TEST_ASSERT_TRUE(device.requestOutputState(kSwitchOutputOn, 201));
    TEST_ASSERT_TRUE(target.state_ == kSwitchOutputOn);

    // Schedule is (or becomes) active while still in manual On mode - entering Auto must not
    // immediately inherit that "on" reading, even though it happens to agree with the manual state.
    schedule.active_ = true;
    DeviceCommand autoCommand{DeviceCommandType::Custom, device.deviceId(), "auto"};
    TEST_ASSERT_TRUE(device.handleCommand(autoCommand));
    device.tick1s(202);
    TEST_ASSERT_TRUE(target.state_ == kSwitchOutputOff);

    // The following tick's normal schedule check then turns it back on.
    device.tick1s(203);
    TEST_ASSERT_TRUE(target.state_ == kSwitchOutputOn);
}

void test_auto_switch_pause_forces_off_until_duration_elapses() {
    AutoSwitchDeviceConfigV1 config = makeAutoSwitchConfig();
    config.pauseDurationSeconds = 10U;
    AutoSwitchDevice device(config);
    FakeSwitch target;
    FakeSchedule schedule;
    bindAutoSwitchIdentity(device, 40U, 41U, {{42U}});
    device.setDependencyRuntime(DeviceRole::Switch, &target);
    device.setDependencyRuntime(DeviceRole::Condition, &schedule);
    startAutoSwitch(device, 1000);

    schedule.active_ = true;
    device.tick1s(1001);
    TEST_ASSERT_TRUE(target.state_ == kSwitchOutputOn);

    DeviceCommand pauseCommand{DeviceCommandType::Custom, device.deviceId(), "pause"};
    TEST_ASSERT_TRUE(device.handleCommand(pauseCommand));
    TEST_ASSERT_TRUE(device.paused());

    // Pause forces the target off immediately, mirroring ReefDuino's PausedMode _toggle(false) -
    // it does not hold whatever Auto last decided.
    device.tick1s(1002);
    TEST_ASSERT_TRUE(target.state_ == kSwitchOutputOff);

    // Schedule turning active again during the pause window must not turn it back on.
    device.tick1s(1000 + 10U * 1000U - 1U);
    TEST_ASSERT_TRUE(target.state_ == kSwitchOutputOff);

    // Pause window elapsed (pausedUntilMs is uptime()+10s from the tick that issued "pause", i.e.
    // from now=1001) - the resume tick itself always forces off first, mirroring ReefDuino's
    // SWITCH_MODE _toggle(false) on entering ScheduledMode, even though the schedule is active.
    const uint32_t resumeTick = 1001 + 10U * 1000U + 1U;
    device.tick1s(resumeTick);
    TEST_ASSERT_FALSE(device.paused());
    TEST_ASSERT_TRUE(target.state_ == kSwitchOutputOff);

    // The following tick's normal schedule check then turns it back on since the schedule is
    // still active.
    device.tick1s(resumeTick + 1U);
    TEST_ASSERT_TRUE(target.state_ == kSwitchOutputOn);
}

void test_auto_switch_pause_command_ignored_unless_currently_in_auto_mode() {
    AutoSwitchDevice device(makeAutoSwitchConfig());
    FakeSwitch target;
    bindAutoSwitchIdentity(device, 45U, 46U);
    device.setDependencyRuntime(DeviceRole::Switch, &target);
    startAutoSwitch(device, 500);

    // Mirrors ReefDuino's pause(), which only takes effect from ScheduledMode - manual On/Off must
    // not be pausable, matching the reference's `if (mode == ScheduledMode)` guard.
    TEST_ASSERT_TRUE(device.requestOutputState(kSwitchOutputOn, 501));
    DeviceCommand pauseCommand{DeviceCommandType::Custom, device.deviceId(), "pause"};
    TEST_ASSERT_FALSE(device.handleCommand(pauseCommand));
    TEST_ASSERT_FALSE(device.paused());
    TEST_ASSERT_TRUE(device.mode() == AutoSwitchMode::On);
}

void test_auto_switch_paused_epoch_math_computes_wall_clock_anchor_from_uptime_deadline() {
    // 10s of pause remain (pausedUntilMs is 10000ms ahead of nowUptimeMs) - the anchor should be
    // nowWallEpoch + 10 seconds.
    const uint32_t anchor =
        AutoSwitchDevice::computePausedUntilEpoch(/*nowUptimeMs=*/5000U, /*pausedUntilMs=*/15000U, /*nowWallEpoch=*/1700000000U);
    TEST_ASSERT_EQUAL_UINT32(1700000010U, anchor);
}

void test_auto_switch_paused_epoch_math_clamps_already_elapsed_deadline_to_now() {
    // pausedUntilMs is already in the past relative to nowUptimeMs - no negative remainder leaking
    // into the anchor.
    const uint32_t anchor =
        AutoSwitchDevice::computePausedUntilEpoch(/*nowUptimeMs=*/20000U, /*pausedUntilMs=*/15000U, /*nowWallEpoch=*/1700000000U);
    TEST_ASSERT_EQUAL_UINT32(1700000000U, anchor);
}

void test_auto_switch_paused_epoch_math_recovers_uptime_deadline_when_anchor_is_in_the_future() {
    uint32_t recovered = 0U;
    // Anchor is 10s ahead of the wall clock read at reload time - the recovered deadline should be
    // 10s ahead of *this boot's* fresh uptime() reading.
    TEST_ASSERT_TRUE(AutoSwitchDevice::recoverPausedUntilMs(/*pausedUntilEpoch=*/1700000010U, /*nowWallEpoch=*/1700000000U,
                                                            /*nowUptimeMs=*/300U, recovered));
    TEST_ASSERT_EQUAL_UINT32(10300U, recovered);
}

void test_auto_switch_paused_epoch_math_rejects_anchor_that_already_elapsed() {
    uint32_t recovered = 12345U;
    TEST_ASSERT_FALSE(AutoSwitchDevice::recoverPausedUntilMs(/*pausedUntilEpoch=*/1700000000U, /*nowWallEpoch=*/1700000005U,
                                                             /*nowUptimeMs=*/300U, recovered));
    // Rejected - outPausedUntilMs must be left untouched.
    TEST_ASSERT_EQUAL_UINT32(12345U, recovered);
}

void test_auto_switch_paused_epoch_math_round_trips_across_a_simulated_reboot() {
    // Simulates a full reboot cycle: 90s of pause remain at save time; 20s of real (wall-clock)
    // time passes while the device is off; reload should recover ~70s remaining, anchored to the
    // new boot's own fresh uptime() reading rather than the old one.
    const uint32_t anchor =
        AutoSwitchDevice::computePausedUntilEpoch(/*nowUptimeMs=*/0U, /*pausedUntilMs=*/90000U, /*nowWallEpoch=*/1700000000U);
    uint32_t recovered = 0U;
    TEST_ASSERT_TRUE(AutoSwitchDevice::recoverPausedUntilMs(anchor, /*nowWallEpoch=*/1700000020U, /*nowUptimeMs=*/500U, recovered));
    TEST_ASSERT_EQUAL_UINT32(500U + 70U * 1000U, recovered);
}

void test_auto_switch_retained_state_round_trip() {
    MemoryConfigStorage storage;
    DeviceRetainedDataStore retainedStore(storage);
    TEST_ASSERT_TRUE(retainedStore.begin(false));

    AutoSwitchDevice device(makeAutoSwitchConfig());
    FakeSwitch target;
    bindAutoSwitchIdentity(device, 50U, 51U);
    device.setDependencyRuntime(DeviceRole::Switch, &target);
    startAutoSwitch(device, 100);

    TEST_ASSERT_TRUE(device.requestOutputState(kSwitchOutputOn, 101));
    TEST_ASSERT_TRUE(device.retainedStateDirty());
    TEST_ASSERT_TRUE(device.saveRetainedState(retainedStore).ok());
    device.clearRetainedStateDirty();
    TEST_ASSERT_FALSE(device.retainedStateDirty());

    AutoSwitchDevice reloaded(makeAutoSwitchConfig());
    FakeSwitch reloadedTarget;
    bindAutoSwitchIdentity(reloaded, 50U, 51U);
    reloaded.setDependencyRuntime(DeviceRole::Switch, &reloadedTarget);
    TEST_ASSERT_TRUE(reloaded.loadRetainedState(retainedStore).ok());
    TEST_ASSERT_TRUE(reloaded.mode() == AutoSwitchMode::On);
}

void test_auto_switch_retained_state_writes_paused_mode_to_the_record() {
    MemoryConfigStorage storage;
    DeviceRetainedDataStore retainedStore(storage);
    TEST_ASSERT_TRUE(retainedStore.begin(false));

    AutoSwitchDeviceConfigV1 config = makeAutoSwitchConfig();
    config.pauseDurationSeconds = 10U;
    AutoSwitchDevice device(config);
    FakeSwitch target;
    FakeSchedule schedule;
    bindAutoSwitchIdentity(device, 55U, 56U, {{57U}});
    device.setDependencyRuntime(DeviceRole::Switch, &target);
    device.setDependencyRuntime(DeviceRole::Condition, &schedule);
    startAutoSwitch(device, 600);

    DeviceCommand pauseCommand{DeviceCommandType::Custom, device.deviceId(), "pause"};
    TEST_ASSERT_TRUE(device.handleCommand(pauseCommand));
    TEST_ASSERT_TRUE(device.paused());
    TEST_ASSERT_TRUE(device.saveRetainedState(retainedStore).ok());

    // Unlike Off/On/Auto, recovering Paused's remaining duration across a reboot needs a synced
    // wall clock (DateTime::current(), Arduino-only) - not exercised on native. What native *can*
    // verify without one is that the record itself carries Paused, not a silently-downgraded Auto.
    AutoSwitchRetainedStateV1 record{};
    TEST_ASSERT_TRUE(retainedStore.load(device.deviceId(), record).ok());
    TEST_ASSERT_EQUAL(static_cast<int>(AutoSwitchMode::Paused), static_cast<int>(record.mode));
}

void test_auto_switch_retained_state_reload_defaults_paused_to_auto_without_wall_clock() {
    MemoryConfigStorage storage;
    DeviceRetainedDataStore retainedStore(storage);
    TEST_ASSERT_TRUE(retainedStore.begin(false));

    AutoSwitchDeviceConfigV1 config = makeAutoSwitchConfig();
    config.pauseDurationSeconds = 10U;
    AutoSwitchDevice device(config);
    FakeSwitch target;
    FakeSchedule schedule;
    bindAutoSwitchIdentity(device, 55U, 56U, {{57U}});
    device.setDependencyRuntime(DeviceRole::Switch, &target);
    device.setDependencyRuntime(DeviceRole::Condition, &schedule);
    startAutoSwitch(device, 600);

    DeviceCommand pauseCommand{DeviceCommandType::Custom, device.deviceId(), "pause"};
    TEST_ASSERT_TRUE(device.handleCommand(pauseCommand));
    TEST_ASSERT_TRUE(device.saveRetainedState(retainedStore).ok());

    // Native/UNIT_TEST builds have no DateTime::current() to recover the persisted deadline from -
    // loadRetainedState() conservatively falls back to Auto rather than resuming paused forever.
    AutoSwitchDevice reloaded(config);
    FakeSwitch reloadedTarget;
    bindAutoSwitchIdentity(reloaded, 55U, 56U, {{57U}});
    reloaded.setDependencyRuntime(DeviceRole::Switch, &reloadedTarget);
    TEST_ASSERT_TRUE(reloaded.loadRetainedState(retainedStore).ok());
    TEST_ASSERT_TRUE(reloaded.mode() == AutoSwitchMode::Auto);
    TEST_ASSERT_FALSE(reloaded.paused());
}

void test_auto_switch_conditions_combine_with_and_and_invert() {
    AutoSwitchDevice device(makeAutoSwitchConfig());
    FakeSwitch target;
    FakeSchedule schedule;
    FakeSwitch overridePin;
    // Condition 0 (schedule, not inverted): satisfied when schedule.active_ is true.
    // Condition 1 (switch, inverted): satisfied when overridePin is Off.
    bindAutoSwitchIdentity(device, 70U, 71U, {{72U, false}, {73U, true}});
    device.setDependencyRuntimeAt(0, &target);
    device.setDependencyRuntimeAt(1, &schedule);
    device.setDependencyRuntimeAt(2, &overridePin);
    startAutoSwitch(device, 300);

    // Both satisfied -> On.
    schedule.active_ = true;
    overridePin.state_ = kSwitchOutputOff;
    device.tick1s(301);
    TEST_ASSERT_TRUE(target.state_ == kSwitchOutputOn);

    // Schedule satisfied but override pin On -> inverted condition fails -> Off.
    overridePin.state_ = kSwitchOutputOn;
    device.tick1s(302);
    TEST_ASSERT_TRUE(target.state_ == kSwitchOutputOff);

    // Override pin back to Off but schedule now inactive -> Off.
    overridePin.state_ = kSwitchOutputOff;
    schedule.active_ = false;
    device.tick1s(303);
    TEST_ASSERT_TRUE(target.state_ == kSwitchOutputOff);
}

void test_auto_switch_condition_count_respects_kMaxAutoSwitchConditions_bound() {
    AutoSwitchDevice device(makeAutoSwitchConfig());
    FakeSwitch target;
    std::array<FakeSwitch, kMaxAutoSwitchConditions> conditionSwitches{};

    std::initializer_list<ConditionDep> conditions = {{80U, false}, {81U, false}, {82U, false}, {83U, false}, {84U, false}, {85U, false}};
    TEST_ASSERT_EQUAL_UINT8(kMaxAutoSwitchConditions, static_cast<uint8_t>(conditions.size()));
    bindAutoSwitchIdentity(device, 79U, 78U, conditions);
    device.setDependencyRuntimeAt(0, &target);
    for (uint8_t index = 0; index < kMaxAutoSwitchConditions; ++index) {
        device.setDependencyRuntimeAt(1 + index, &conditionSwitches[index]);
    }
    startAutoSwitch(device, 400);

    for (auto& conditionSwitch : conditionSwitches) {
        conditionSwitch.state_ = kSwitchOutputOn;
    }
    device.tick1s(401);
    TEST_ASSERT_TRUE(target.state_ == kSwitchOutputOn);

    // A single unsatisfied condition among the full set is enough to force Off.
    conditionSwitches[kMaxAutoSwitchConditions - 1U].state_ = kSwitchOutputOff;
    device.tick1s(402);
    TEST_ASSERT_TRUE(target.state_ == kSwitchOutputOff);
}

void test_auto_switch_api_adapter_rejects_duplicate_condition_device_id_on_create() {
    StaticJsonDocument<512> doc;
    JsonObject input = doc.to<JsonObject>();
    JsonObject config = input.createNestedObject("config");
    config["name"] = "auto_switch";
    config["enabled"] = true;
    JsonArray deps = config.createNestedArray("deps");
    JsonObject switchDep = deps.createNestedObject();
    switchDep["role"] = "switch";
    switchDep["deviceId"] = 10;
    JsonObject conditionDepA = deps.createNestedObject();
    conditionDepA["role"] = "condition";
    conditionDepA["deviceId"] = 20;
    JsonObject conditionDepB = deps.createNestedObject();
    conditionDepB["role"] = "condition";
    conditionDepB["deviceId"] = 20; // same device as conditionDepA

    DeviceCreateRequest request{};
    const char* error = nullptr;
    TEST_ASSERT_FALSE(AutoSwitchDeviceApiAdapter::instance().parseCreateRequest(input, request, error));
    TEST_ASSERT_EQUAL_STRING("auto switch dependency device id is duplicated", error);
}

void test_auto_switch_api_adapter_rejects_target_switch_reused_as_condition_on_create() {
    StaticJsonDocument<512> doc;
    JsonObject input = doc.to<JsonObject>();
    JsonObject config = input.createNestedObject("config");
    config["name"] = "auto_switch";
    config["enabled"] = true;
    JsonArray deps = config.createNestedArray("deps");
    JsonObject switchDep = deps.createNestedObject();
    switchDep["role"] = "switch";
    switchDep["deviceId"] = 10;
    JsonObject conditionDep = deps.createNestedObject();
    conditionDep["role"] = "condition";
    conditionDep["deviceId"] = 10; // same device as the target switch

    DeviceCreateRequest request{};
    const char* error = nullptr;
    TEST_ASSERT_FALSE(AutoSwitchDeviceApiAdapter::instance().parseCreateRequest(input, request, error));
    TEST_ASSERT_EQUAL_STRING("auto switch dependency device id is duplicated", error);
}

void test_auto_switch_api_adapter_set_deps_rejects_duplicate_condition_device_id() {
    AutoSwitchDevice device(makeAutoSwitchConfig());
    bindAutoSwitchIdentity(device, 90U, 91U);

    std::array<DeviceDependencyLink, kMaxDeviceDependencies> deps{};
    uint8_t depCount = 0;
    deps[depCount++] = DeviceDependencyLink{DeviceRole::Switch, 91U, false};
    deps[depCount++] = DeviceDependencyLink{DeviceRole::Condition, 92U, false};
    deps[depCount++] = DeviceDependencyLink{DeviceRole::Condition, 92U, true};

    MemoryConfigStorage storage;
    DeviceRegistryStore store(storage);
    TEST_ASSERT_TRUE(store.begin(false));
    SequentialDeviceIdSource ids(1000);
    DeviceTypeRegistry types = DeviceTypeRegistry::withDefaults();
    DeviceRegistry registry(store, types, ids);
    TEST_ASSERT_TRUE(registry.begin(0).ok());

    const DeviceValidationResult result = AutoSwitchDeviceApiAdapter::instance().validateSetDepsRequest(device, deps, depCount, registry);
    TEST_ASSERT_FALSE(result.ok());
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceError::InvalidRelationship), static_cast<int>(result.error));
    TEST_ASSERT_EQUAL_STRING("auto switch dependency device id is duplicated", result.message);
}
