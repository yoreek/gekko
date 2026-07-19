#include "config/MemoryConfigStorage.h"
#include "devices/analog/AnalogOutputDeviceBase.h"
#include "devices/analog/AnalogOutputDeviceConfig.h"
#include "devices/analog/composer/AnalogOutputComposerDevice.h"
#include "devices/analog/fade/FadeAnalogOutputDevice.h"
#include "devices/analog/scheduled/ScheduledAnalogOutputDevice.h"
#include "devices/registry/DeviceRetainedDataStore.h"
#include "integrations/common/DeviceApiAdapter.h"

#include <cstdio>
#include <unity.h>

using namespace ewfm;

namespace {

class FakeAnalogOutputDevice final : public AnalogOutputDeviceBase, public IScheduledAnalogOutputRuntime {
public:
    explicit FakeAnalogOutputDevice(const char* name) : AnalogOutputDeviceBase(config_) {
        config_.enabled = 1U;
        std::snprintf(config_.name, sizeof(config_.name), "%s", name);
    }

    const IScheduledAnalogOutputRuntime* scheduledAnalogOutputRuntime() const override {
        return this;
    }

    AnalogOutputMode analogOutputMode() const override {
        return mode_;
    }

    bool requestAnalogOutputMode(const AnalogOutputMode mode, uint32_t now) override {
        if (status() != DeviceStatus::Ready) {
            return false;
        }
        mode_ = mode;
        return mode != AnalogOutputMode::Off || requestOutputState(0U, now);
    }

    uint16_t requestedAnalogOutputState() const override {
        return currentOutputState();
    }

    bool analogOutputTimeValid() const override {
        return true;
    }

private:
    const AnalogOutputDeviceConfigV1& config() const override {
        return config_;
    }

    DeviceValidationResult configureHardware(uint32_t now) override {
        (void)now;
        return {};
    }

    DeviceValidationResult applyHardwareOutput(uint16_t state, uint32_t now) override {
        (void)state;
        (void)now;
        return {};
    }

    void releaseHardware(uint32_t now) override {
        (void)now;
    }

    AnalogOutputDeviceConfigV1 config_{};
    AnalogOutputMode mode_{AnalogOutputMode::Scheduled};
};

DeviceRegistryEntry dependentRecord(const DeviceTypeId typeId, const std::initializer_list<DeviceId> dependencies) {
    DeviceRegistryEntry record{};
    record.header.deviceId = typeId + 100U;
    record.header.typeId = typeId;
    for (const DeviceId dependency : dependencies) {
        record.deps[record.depCount++] = DeviceDependencyLink{DeviceRole::AnalogOutput, dependency};
    }
    return record;
}

ScheduledAnalogOutputDeviceConfigV2 scheduleConfig() {
    ScheduledAnalogOutputDeviceConfigV2 config{};
    config.enabled = 1U;
    std::snprintf(config.name, sizeof(config.name), "%s", "schedule");
    config.points[0] = {0U, 60U, 0U};
    config.points[1] = {0U, 120U, kAnalogOutputLevelMax};
    config.points[2] = {0U, 1380U, 0U};
    return config;
}

} // namespace

void test_fade_analog_output_advances_in_bounded_cooperative_steps() {
    FakeAnalogOutputDevice target("target");
    target.begin(1U);
    target.tickFastLoop(2U);

    FadeAnalogOutputDeviceConfigV1 config{};
    config.enabled = 1U;
    config.maxStep = percentToAnalogOutputState(10U);
    config.stepIntervalMs = 200U;
    FadeAnalogOutputDevice fade(config);
    const DeviceRegistryEntry record = dependentRecord(kFadeAnalogOutputDeviceTypeId, {1U});
    fade.bindDeviceIdentity(record, {});
    fade.setDependencyRuntimeAt(0U, &target);
    fade.begin(10U);
    fade.tick100ms(11U);
    fade.tick100ms(12U);

    TEST_ASSERT_EQUAL(static_cast<int>(DeviceStatus::Ready), static_cast<int>(fade.status()));
    TEST_ASSERT_TRUE(fade.requestOutputState(kAnalogOutputLevelMax, 20U));
    fade.tick100ms(212U);
    TEST_ASSERT_EQUAL_UINT16(config.maxStep, target.currentOutputState());
    TEST_ASSERT_EQUAL_UINT16(kAnalogOutputLevelMax, fade.targetOutputState());
    TEST_ASSERT_TRUE(fade.transitioning());
}

void test_composable_analog_output_configs_round_trip_json_and_validate() {
    FadeAnalogOutputDeviceConfigV1 fade{};
    fade.enabled = 1U;
    std::snprintf(fade.name, sizeof(fade.name), "%s", "fade");
    fade.maxStep = percentToAnalogOutputState(10U);
    TEST_ASSERT_TRUE(fade.validate().ok());
    StaticJsonDocument<256> fadeJson;
    fade.writeJson(fadeJson.to<JsonObject>());
    TEST_ASSERT_EQUAL_UINT8(10U, fadeJson["maxStep"].as<uint8_t>());
    TEST_ASSERT_EQUAL_UINT32(kDefaultFadeAnalogOutputStepIntervalMs, fadeJson["stepIntervalMs"].as<uint32_t>());
    const char* error = nullptr;
    FadeAnalogOutputDeviceConfigV1 parsedFade{};
    TEST_ASSERT_TRUE(parsedFade.parseJson(fadeJson.as<JsonObjectConst>(), error));
    TEST_ASSERT_EQUAL_UINT16(fade.maxStep, parsedFade.maxStep);

    ScheduledAnalogOutputDeviceConfigV2 scheduled = scheduleConfig();
    TEST_ASSERT_TRUE(scheduled.validate().ok());
    StaticJsonDocument<768> scheduleJson;
    scheduled.writeJson(scheduleJson.to<JsonObject>());
    TEST_ASSERT_EQUAL_UINT8(3U, scheduleJson["points"].as<JsonArrayConst>().size());
    TEST_ASSERT_EQUAL_UINT8(100U, scheduleJson["points"][1]["state"].as<uint8_t>());
    ScheduledAnalogOutputDeviceConfigV2 parsedSchedule{};
    TEST_ASSERT_TRUE(parsedSchedule.parseJson(scheduleJson.as<JsonObjectConst>(), error));
    TEST_ASSERT_EQUAL_UINT16(scheduled.points[1].state, parsedSchedule.points[1].state);

    StaticJsonDocument<128> emptyScheduleJson;
    emptyScheduleJson["name"] = "schedule";
    emptyScheduleJson.createNestedArray("points");
    ScheduledAnalogOutputDeviceConfigV2 rejectedEmptySchedule{};
    TEST_ASSERT_FALSE(rejectedEmptySchedule.parseJson(emptyScheduleJson.as<JsonObjectConst>(), error));
    TEST_ASSERT_EQUAL_STRING("analog schedule requires at least one point", error);

    ScheduledAnalogOutputDeviceConfigV1 legacyEmptySchedule{};
    legacyEmptySchedule.enabled = 1U;
    std::snprintf(legacyEmptySchedule.name, sizeof(legacyEmptySchedule.name), "%s", "legacy schedule");
    uint8_t legacyBlob[kMaxDeviceConfigBytes]{};
    const size_t legacySize = scheduledAnalogOutputDeviceConfigSize(legacyEmptySchedule);
    TEST_ASSERT_TRUE(encodeFixedConfigBlob(ScheduledAnalogOutputDeviceConfigV1::kMagic, legacyEmptySchedule, legacyBlob, legacySize));
    ScheduledAnalogOutputDeviceConfigV2 migratedSchedule{};
    TEST_ASSERT_TRUE(decodeScheduledAnalogOutputDeviceConfig(legacyBlob, legacySize, migratedSchedule));
    TEST_ASSERT_EQUAL_UINT8(0U, migratedSchedule.points[0].deleted);
    TEST_ASSERT_EQUAL_UINT16(kAnalogOutputLevelMax, migratedSchedule.points[0].state);

    scheduled.points[2].minuteOfDay = scheduled.points[1].minuteOfDay;
    TEST_ASSERT_FALSE(scheduled.validate().ok());
    scheduled.points[2].deleted = 1U;
    TEST_ASSERT_TRUE(scheduled.validate().ok());

    AnalogOutputComposerDeviceConfigV1 composer{};
    composer.enabled = 1U;
    std::snprintf(composer.name, sizeof(composer.name), "%s", "composer");
    TEST_ASSERT_TRUE(composer.validate().ok());
}

void test_scheduled_analog_output_interpolates_across_midnight() {
    const ScheduledAnalogOutputDeviceConfigV2 config = scheduleConfig();
    bool hasPoints = false;

    TEST_ASSERT_EQUAL_UINT16((kAnalogOutputLevelMax + 1U) / 2U, ScheduledAnalogOutputDevice::scheduledStateAt(config, 90U, hasPoints));
    TEST_ASSERT_TRUE(hasPoints);
    TEST_ASSERT_EQUAL_UINT16(0U, ScheduledAnalogOutputDevice::scheduledStateAt(config, 0U, hasPoints));
    TEST_ASSERT_EQUAL_UINT16(0U, ScheduledAnalogOutputDevice::scheduledStateAt(config, 1410U, hasPoints));

    ScheduledAnalogOutputDeviceConfigV2 defaults{};
    defaults.enabled = 1U;
    std::snprintf(defaults.name, sizeof(defaults.name), "%s", "defaults");
    TEST_ASSERT_TRUE(defaults.validate().ok());
    TEST_ASSERT_EQUAL_UINT16(kAnalogOutputLevelMax, ScheduledAnalogOutputDevice::scheduledStateAt(defaults, 600U, hasPoints));
    TEST_ASSERT_TRUE(hasPoints);

    FakeAnalogOutputDevice target("scheduled-target");
    target.begin(1U);
    target.tickFastLoop(2U);
    ScheduledAnalogOutputDevice scheduled(config);
    const DeviceRegistryEntry record = dependentRecord(kScheduledAnalogOutputDeviceTypeId, {1U});
    scheduled.bindDeviceIdentity(record, {});
    scheduled.setDependencyRuntimeAt(0U, &target);
    scheduled.begin(10U);
    scheduled.tick1s(11U);
    scheduled.tick1s(12U);
    TEST_ASSERT_TRUE(scheduled.requestOutputState(percentToAnalogOutputState(50U), 13U));
    TEST_ASSERT_TRUE(scheduled.requestAnalogOutputMode(AnalogOutputMode::Off, 14U));
    TEST_ASSERT_EQUAL(static_cast<int>(AnalogOutputMode::Off), static_cast<int>(scheduled.analogOutputMode()));
    TEST_ASSERT_EQUAL_UINT16(0U, scheduled.requestedAnalogOutputState());
    TEST_ASSERT_EQUAL_UINT16(0U, target.currentOutputState());
}

void test_analog_output_composer_propagates_group_mode() {
    FakeAnalogOutputDevice first("first");
    FakeAnalogOutputDevice second("second");
    first.begin(1U);
    first.tickFastLoop(2U);
    second.begin(1U);
    second.tickFastLoop(2U);
    TEST_ASSERT_TRUE(first.requestOutputState(percentToAnalogOutputState(20U), 3U));
    TEST_ASSERT_TRUE(second.requestOutputState(percentToAnalogOutputState(40U), 3U));

    AnalogOutputComposerDeviceConfigV1 config{};
    config.enabled = 1U;
    AnalogOutputComposerDevice composer(config);
    const DeviceRegistryEntry record = dependentRecord(kAnalogOutputComposerDeviceTypeId, {1U, 2U});
    composer.bindDeviceIdentity(record, {});
    composer.setDependencyRuntimeAt(0U, &first);
    composer.setDependencyRuntimeAt(1U, &second);
    composer.begin(10U);
    composer.tick1s(11U);
    composer.tick1s(12U);

    TEST_ASSERT_EQUAL(static_cast<int>(DeviceStatus::Ready), static_cast<int>(composer.status()));
    TEST_ASSERT_EQUAL(static_cast<int>(AnalogOutputMode::Scheduled), static_cast<int>(composer.analogOutputGroupMode()));
    TEST_ASSERT_TRUE(composer.requestAnalogOutputGroupMode(AnalogOutputMode::Off, 19U));
    TEST_ASSERT_EQUAL(static_cast<int>(AnalogOutputMode::Off), static_cast<int>(composer.analogOutputGroupMode()));
    TEST_ASSERT_EQUAL(static_cast<int>(AnalogOutputMode::Off), static_cast<int>(first.analogOutputMode()));
    TEST_ASSERT_TRUE(composer.requestAnalogOutputGroupMode(AnalogOutputMode::Manual, 20U));
    TEST_ASSERT_EQUAL(static_cast<int>(AnalogOutputMode::Manual), static_cast<int>(composer.analogOutputGroupMode()));
    TEST_ASSERT_EQUAL(static_cast<int>(AnalogOutputMode::Manual), static_cast<int>(first.analogOutputMode()));
    TEST_ASSERT_EQUAL(static_cast<int>(AnalogOutputMode::Manual), static_cast<int>(second.analogOutputMode()));

    TEST_ASSERT_TRUE(composer.requestAnalogOutputGroupMode(AnalogOutputMode::Scheduled, 21U));
    TEST_ASSERT_EQUAL(static_cast<int>(AnalogOutputMode::Scheduled), static_cast<int>(first.analogOutputMode()));
    TEST_ASSERT_TRUE(first.requestAnalogOutputMode(AnalogOutputMode::Manual, 22U));
    composer.tick1s(23U);
    TEST_ASSERT_EQUAL(static_cast<int>(AnalogOutputMode::Scheduled), static_cast<int>(first.analogOutputMode()));

    MemoryConfigStorage storage;
    DeviceRetainedDataStore retainedStore(storage);
    TEST_ASSERT_TRUE(retainedStore.begin(false));
    TEST_ASSERT_TRUE(composer.requestAnalogOutputGroupMode(AnalogOutputMode::Off, 24U));
    TEST_ASSERT_TRUE(composer.retainedStateDirty());
    TEST_ASSERT_TRUE(composer.saveRetainedState(retainedStore).ok());

    AnalogOutputComposerDevice restored(config);
    restored.bindDeviceIdentity(record, {});
    TEST_ASSERT_TRUE(restored.loadRetainedState(retainedStore).ok());
    TEST_ASSERT_EQUAL(static_cast<int>(AnalogOutputMode::Off), static_cast<int>(restored.analogOutputGroupMode()));
}

void test_composable_analog_output_types_and_api_adapters_are_registered() {
    DeviceTypeRegistry types = DeviceTypeRegistry::withDefaults();
    DeviceApiAdapterRegistry adapters = DeviceApiAdapterRegistry::withDefaults();
    const DeviceTypeId typeIds[] = {
        kFadeAnalogOutputDeviceTypeId,
        kScheduledAnalogOutputDeviceTypeId,
        kAnalogOutputComposerDeviceTypeId,
    };
    for (const DeviceTypeId typeId : typeIds) {
        TEST_ASSERT_NOT_NULL(types.find(typeId));
        TEST_ASSERT_NOT_NULL(adapters.find(typeId));
    }
    TEST_ASSERT_TRUE(types.find(kFadeAnalogOutputDeviceTypeId)->providedRoles.contains(DeviceRole::AnalogOutput));
    TEST_ASSERT_TRUE(types.find(kScheduledAnalogOutputDeviceTypeId)->providedRoles.contains(DeviceRole::AnalogOutput));
    TEST_ASSERT_TRUE(types.find(kAnalogOutputComposerDeviceTypeId)->providedRoles.contains(DeviceRole::AnalogOutputGroup));
    TEST_ASSERT_TRUE(types.find(kAnalogOutputComposerDeviceTypeId)->supportsRetainedState);
}
