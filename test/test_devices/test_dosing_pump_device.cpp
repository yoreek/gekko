#include "config/MemoryConfigStorage.h"
#include "devices/core/DeviceTypes.h"
#include "devices/dosing/DosingPumpDevice.h"
#include "devices/dosing/DosingPumpDeviceConfig.h"
#include "devices/dosing/DosingPumpSchedule.h"
#include "devices/dosing/journal/DoseJournal.h"
#include "devices/registry/DeviceRetainedDataStore.h"
#include "devices/switch/SwitchOutputState.h"
#include "integrations/rest/dosing_pump/DosingPumpDeviceApiAdapter.h"
#include "time/DateTime.h"

#include <algorithm>
#include <cstdio>
#include <unity.h>
#include <vector>

using namespace ewfm;

namespace {

class FakePumpSwitch final : public DeviceRuntimeBase, public ISwitchOutputRuntime {
public:
    using StateType = ISwitchOutputRuntime::StateType;

    FakePumpSwitch() : DeviceRuntimeBase((PState)&FakePumpSwitch::Idle) {
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

    StateType state_{false};
    uint32_t requestCount{0U};
    bool requestOk{true};

private:
    State Idle() {
        status_ = DeviceStatus::Ready;
    }
};

class FakeLevelSensor final : public DeviceRuntimeBase, public IStatusRuntime {
public:
    FakeLevelSensor() : DeviceRuntimeBase((PState)&FakeLevelSensor::Idle) {
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
    const IStatusRuntime* statusRuntime() const override {
        return this;
    }
    bool isActive() const override {
        return active_;
    }

    bool active_{false};

private:
    State Idle() {
        status_ = DeviceStatus::Ready;
    }
};

class MemoryJournal final : public IDoseJournal {
public:
    bool append(const DoseJournalRecordV1& record) override {
        records.push_back(record);
        return true;
    }
    void forEachNewestFirst(uint32_t deviceId, uint32_t sinceEpoch, Visitor visitor, void* context) const override {
        for (auto it = records.rbegin(); it != records.rend(); ++it) {
            if (deviceId != 0U && it->deviceId != deviceId) {
                continue;
            }
            if (it->epoch < sinceEpoch) {
                continue;
            }
            if (!visitor(*it, context)) {
                return;
            }
        }
    }
    bool removeDevice(uint32_t deviceId) override {
        records.erase(std::remove_if(records.begin(), records.end(),
                                     [deviceId](const DoseJournalRecordV1& record) { return record.deviceId == deviceId; }),
                      records.end());
        return true;
    }

    std::vector<DoseJournalRecordV1> records{};
};

// Restores the process-global journal seam on scope exit so a failing test cannot leave a
// dangling pointer behind for the next one.
struct ScopedJournal {
    explicit ScopedJournal(IDoseJournal* journal) {
        setDefaultDoseJournal(journal);
    }
    ~ScopedJournal() {
        setDefaultDoseJournal(nullptr);
    }
};

DosingPumpDeviceConfigV1 makeDosingPumpConfig() {
    DosingPumpDeviceConfigV1 config{};
    config.enabled = 1U;
    std::snprintf(config.name, sizeof(config.name), "%s", "calcium pump");
    config.speedMilliMlPerSec = 1000U; // 1 ml/s
    config.containerCapacityMl = 1000U;
    config.thresholdPercent = 10U;
    config.blockAutoWhenEmpty = 1U;
    config.scheduleMode = static_cast<uint8_t>(DosingScheduleMode::Daily);
    config.everyDays = 1U;
    config.daysOfWeekMask = 0x7F;
    return config;
}

BoundedBlob<kMaxDeviceConfigBytes> encodeDosingPumpPayload(const DosingPumpDeviceConfigV1& config) {
    BoundedBlob<kMaxDeviceConfigBytes> payload{};
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    TEST_ASSERT_TRUE(encodeFixedConfigBlob(DosingPumpDeviceConfigV1::kMagic, config, buffer, dosingPumpDeviceConfigSize(config)));
    TEST_ASSERT_TRUE(payload.assign(buffer, dosingPumpDeviceConfigSize(config)));
    return payload;
}

void bindDosingPumpIdentity(DosingPumpDevice& device, DeviceId pumpId, DeviceId switchId, DeviceId sensorId = 0U,
                            bool sensorInvert = false) {
    const BoundedBlob<kMaxDeviceConfigBytes> configBlob = encodeDosingPumpPayload(device.config());
    DeviceRegistryEntry record{};
    record.header.deviceId = pumpId;
    record.header.typeId = kDosingPumpDeviceTypeId;
    record.header.configVersion = kDosingPumpDeviceConfigVersion;
    record.header.configRevision = 1U;
    record.header.payloadLength = static_cast<uint32_t>(configBlob.size());
    record.deps[0] = DeviceDependencyLink{DeviceRole::Switch, switchId, false};
    uint8_t depCount = 1U;
    if (sensorId != 0U) {
        record.deps[depCount++] = DeviceDependencyLink{DeviceRole::Condition, sensorId, sensorInvert};
    }
    record.depCount = depCount;
    record.status = DeviceStatus::Ready;
    device.bindDeviceIdentity(record, configBlob);
}

void startDosingPump(DosingPumpDevice& device, uint32_t now = 10U) {
    device.begin(now);
    device.tick100ms(now + 1U);
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceStatus::Ready), static_cast<int>(device.status()));
}

DeviceCommand customCommand(const char* payload) {
    return DeviceCommand{DeviceCommandType::Custom, 1U, payload};
}

// 2026-07-13 is a Monday.
DateTime mondayAt(uint8_t hour, uint8_t minute, uint8_t second = 0U) {
    return DateTime(2026U, 7U, 13U, hour, minute, second);
}

} // namespace

void test_dosing_pump_config_codec_round_trip_and_validation() {
    DosingPumpDeviceConfigV1 config = makeDosingPumpConfig();
    config.doses[0] = DosingPumpDoseV1{8U * 60U, 1230U};
    config.doses[1] = DosingPumpDoseV1{20U * 60U, 500U};
    config.doseCount = 2U;
    config.anchorDay = 20647U;

    uint8_t buffer[kMaxDeviceConfigBytes]{};
    const size_t size = dosingPumpDeviceConfigSize(config);
    TEST_ASSERT_TRUE(encodeFixedConfigBlob(DosingPumpDeviceConfigV1::kMagic, config, buffer, size));

    DosingPumpDeviceConfigV1 decoded{};
    TEST_ASSERT_TRUE(decodeValidatedFixedConfigBlob(DosingPumpDeviceConfigV1::kMagic, buffer, size, decoded));
    TEST_ASSERT_EQUAL_UINT16(1000U, decoded.speedMilliMlPerSec);
    TEST_ASSERT_EQUAL_UINT16(1000U, decoded.containerCapacityMl);
    TEST_ASSERT_EQUAL_UINT8(2U, decoded.doseCount);
    TEST_ASSERT_EQUAL_UINT16(1230U, decoded.doses[0].amountCentiMl);
    TEST_ASSERT_EQUAL_UINT16(20647U, decoded.anchorDay);
    TEST_ASSERT_TRUE(decoded.validate().ok());
}

void test_dosing_pump_config_rejects_unsorted_doses_and_empty_weekly() {
    DosingPumpDeviceConfigV1 config = makeDosingPumpConfig();
    config.doses[0] = DosingPumpDoseV1{600U, 100U};
    config.doses[1] = DosingPumpDoseV1{600U, 100U}; // duplicate time
    config.doseCount = 2U;
    TEST_ASSERT_FALSE(config.validate().ok());

    config.doses[1] = DosingPumpDoseV1{300U, 100U}; // out of order
    TEST_ASSERT_FALSE(config.validate().ok());

    config.doseCount = 1U;
    config.scheduleMode = static_cast<uint8_t>(DosingScheduleMode::Weekly);
    config.daysOfWeekMask = 0U;
    TEST_ASSERT_FALSE(config.validate().ok());
}

void test_dosing_pump_type_and_api_adapter_are_registered() {
    DeviceTypeRegistry types = DeviceTypeRegistry::withDefaults();
    const DeviceTypeDescriptor* descriptor = types.find(kDosingPumpDeviceTypeId);
    TEST_ASSERT_NOT_NULL(descriptor);
    TEST_ASSERT_EQUAL_STRING("DosingPumpDevice", descriptor->name);
    TEST_ASSERT_TRUE(descriptor->ticks100ms);
    TEST_ASSERT_TRUE(descriptor->supportsCommands);
    TEST_ASSERT_TRUE(descriptor->supportsRetainedState);

    DeviceApiAdapterRegistry adapters = DeviceApiAdapterRegistry::withDefaults();
    const IDeviceApiAdapter* adapter = adapters.find(kDosingPumpDeviceTypeId);
    TEST_ASSERT_NOT_NULL(adapter);
    TEST_ASSERT_EQUAL_STRING("dosing_pump", adapter->typeName());
}

void test_dosing_pump_day_active_follows_every_days_anchor_and_weekly_mask() {
    DosingPumpDeviceConfigV1 config = makeDosingPumpConfig();
    config.everyDays = 3U;
    config.anchorDay = 20000U;
    TEST_ASSERT_TRUE(dosingDayActive(config, 20000U));
    TEST_ASSERT_FALSE(dosingDayActive(config, 20001U));
    TEST_ASSERT_FALSE(dosingDayActive(config, 20002U));
    TEST_ASSERT_TRUE(dosingDayActive(config, 20003U));
    // A browser-clock anchor one day ahead of the device still yields a well-defined phase.
    TEST_ASSERT_TRUE(dosingDayActive(config, 19997U));

    config.scheduleMode = static_cast<uint8_t>(DosingScheduleMode::Weekly);
    config.daysOfWeekMask = static_cast<uint8_t>(1U << 1U); // Mondays only
    const uint32_t monday = mondayAt(0U, 0U).unixtime() / 86400UL;
    TEST_ASSERT_TRUE(dosingDayActive(config, monday));
    TEST_ASSERT_FALSE(dosingDayActive(config, monday + 1U));
    TEST_ASSERT_TRUE(dosingDayActive(config, monday + 7U));
}

void test_dosing_pump_due_and_missed_masks_respect_grace_window() {
    DosingPumpDeviceConfigV1 config = makeDosingPumpConfig();
    config.doses[0] = DosingPumpDoseV1{480U, 100U};
    config.doses[1] = DosingPumpDoseV1{600U, 200U};
    config.doseCount = 2U;

    TEST_ASSERT_EQUAL(-1, dueDoseIndex(config, 479U, 0U));
    TEST_ASSERT_EQUAL(0, dueDoseIndex(config, 480U, 0U));
    TEST_ASSERT_EQUAL(0, dueDoseIndex(config, 484U, 0U));
    TEST_ASSERT_EQUAL(-1, dueDoseIndex(config, 485U, 0U));
    TEST_ASSERT_EQUAL(-1, dueDoseIndex(config, 481U, 0x1U)); // already fired

    TEST_ASSERT_EQUAL_UINT16(0U, missedDosesMask(config, 484U));
    TEST_ASSERT_EQUAL_UINT16(0x1U, missedDosesMask(config, 485U));
    TEST_ASSERT_EQUAL_UINT16(0x3U, missedDosesMask(config, 605U));
}

void test_dosing_pump_next_dose_scans_days_and_consumes_skips() {
    DosingPumpDeviceConfigV1 config = makeDosingPumpConfig();
    config.doses[0] = DosingPumpDoseV1{480U, 100U};
    config.doses[1] = DosingPumpDoseV1{600U, 200U};
    config.doseCount = 2U;

    uint32_t epoch = 0U;
    uint16_t amount = 0U;

    // Later today.
    TEST_ASSERT_TRUE(nextDose(config, 100U, 500U, 0x1U, 0U, epoch, amount));
    TEST_ASSERT_EQUAL_UINT32(100UL * 86400UL + 600UL * 60UL, epoch);
    TEST_ASSERT_EQUAL_UINT16(200U, amount);

    // Everything fired today -> first dose tomorrow.
    TEST_ASSERT_TRUE(nextDose(config, 100U, 700U, 0x3U, 0U, epoch, amount));
    TEST_ASSERT_EQUAL_UINT32(101UL * 86400UL + 480UL * 60UL, epoch);

    // Skip flag suppresses the first occurrence only.
    TEST_ASSERT_TRUE(nextDose(config, 100U, 500U, 0x1U, 0x2U, epoch, amount));
    TEST_ASSERT_EQUAL_UINT32(101UL * 86400UL + 480UL * 60UL, epoch);
    TEST_ASSERT_EQUAL_UINT16(100U, amount);

    // everyDays gap: today inactive, next active day is the anchor phase.
    config.everyDays = 3U;
    config.anchorDay = 99U;
    TEST_ASSERT_TRUE(nextDose(config, 100U, 0U, 0U, 0U, epoch, amount));
    TEST_ASSERT_EQUAL_UINT32(102UL * 86400UL + 480UL * 60UL, epoch);
}

void test_dosing_pump_run_time_math_round_trips() {
    TEST_ASSERT_EQUAL_UINT32(5000U, doseRunMs(500U, 1000U));  // 5 ml @ 1 ml/s -> 5 s
    TEST_ASSERT_EQUAL_UINT32(98400U, doseRunMs(1230U, 125U)); // 12.3 ml @ 0.125 ml/s -> 98.4 s
    TEST_ASSERT_EQUAL_UINT32(0U, doseRunMs(100U, 0U));
    TEST_ASSERT_EQUAL_UINT32(500U, dosedCentiMl(5000U, 1000U));
    TEST_ASSERT_EQUAL_UINT32(250U, dosedCentiMl(2500U, 1000U));
    TEST_ASSERT_EQUAL_UINT32(1230U, dosedCentiMl(98400U, 125U));

    DosingPumpDeviceConfigV1 config = makeDosingPumpConfig();
    config.doses[0] = DosingPumpDoseV1{480U, 300U};
    config.doses[1] = DosingPumpDoseV1{600U, 300U};
    config.doseCount = 2U;
    TEST_ASSERT_EQUAL_UINT32(600U, todayTargetCentiMl(config, true));
    TEST_ASSERT_EQUAL_UINT32(0U, todayTargetCentiMl(config, false));
    TEST_ASSERT_EQUAL_UINT32(600U, averageDailyCentiMl(config));
    config.everyDays = 3U;
    TEST_ASSERT_EQUAL_UINT32(200U, averageDailyCentiMl(config));
    config.scheduleMode = static_cast<uint8_t>(DosingScheduleMode::Weekly);
    config.daysOfWeekMask = 0x03U; // 2 of 7 days
    TEST_ASSERT_EQUAL_UINT32(600U * 2U / 7U, averageDailyCentiMl(config));
}

void test_dosing_pump_manual_dose_runs_to_completion_and_logs() {
    MemoryJournal journal;
    ScopedJournal scopedJournal(&journal);

    DosingPumpDevice device(makeDosingPumpConfig());
    FakePumpSwitch pump;
    bindDosingPumpIdentity(device, 1U, 2U);
    device.setDependencyRuntime(DeviceRole::Switch, &pump);
    startDosingPump(device);
    device.evaluateSchedule(mondayAt(9U, 0U), 11U); // arms timeValid + wall epoch for the journal

    // 5 ml @ 1 ml/s -> 5 s run.
    TEST_ASSERT_TRUE(device.handleCommand(customCommand("startDose:500:1")));
    TEST_ASSERT_TRUE(device.running());
    TEST_ASSERT_TRUE(pump.state_ == kSwitchOutputOn);

    device.tick100ms(3000U);
    TEST_ASSERT_TRUE(device.running());
    device.tick100ms(11U + 5000U);
    TEST_ASSERT_FALSE(device.running());
    TEST_ASSERT_TRUE(pump.state_ == kSwitchOutputOff);
    TEST_ASSERT_EQUAL_UINT32(500U, device.lastRunDosedCentiMl());
    TEST_ASSERT_EQUAL_UINT32(500U, device.todayDosedCentiMl());
    TEST_ASSERT_EQUAL_UINT32(100000U - 500U, device.containerCurrentCentiMl());
    TEST_ASSERT_EQUAL_UINT16(500U, device.lastDoseAmountCentiMl());
    TEST_ASSERT_EQUAL(1U, journal.records.size());
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(DoseJournalEntryType::Manual), journal.records[0].type);
    TEST_ASSERT_EQUAL_UINT16(500U, journal.records[0].amountCentiMl);
    TEST_ASSERT_EQUAL_UINT32(1U, journal.records[0].deviceId);
}

void test_dosing_pump_early_stop_counts_partial_amount_and_busy_rejects() {
    DosingPumpDevice device(makeDosingPumpConfig());
    FakePumpSwitch pump;
    bindDosingPumpIdentity(device, 1U, 2U);
    device.setDependencyRuntime(DeviceRole::Switch, &pump);
    startDosingPump(device);

    TEST_ASSERT_TRUE(device.handleCommand(customCommand("startDose:1000:1"))); // 10 s run
    TEST_ASSERT_FALSE(device.handleCommand(customCommand("startDose:100:1"))); // busy

    device.tick100ms(11U + 4000U); // 4 s in
    TEST_ASSERT_TRUE(device.stopRun(11U + 4000U));
    TEST_ASSERT_FALSE(device.running());
    TEST_ASSERT_TRUE(pump.state_ == kSwitchOutputOff);
    TEST_ASSERT_EQUAL_UINT32(400U, device.lastRunDosedCentiMl()); // 4 ml of 10 ml
    TEST_ASSERT_EQUAL_UINT32(400U, device.todayDosedCentiMl());
    TEST_ASSERT_TRUE(device.stopRun(11U + 4100U)); // idempotent
}

void test_dosing_pump_calibration_run_is_not_logged_but_consumes_container() {
    MemoryJournal journal;
    ScopedJournal scopedJournal(&journal);

    DosingPumpDevice device(makeDosingPumpConfig());
    FakePumpSwitch pump;
    bindDosingPumpIdentity(device, 1U, 2U);
    device.setDependencyRuntime(DeviceRole::Switch, &pump);
    startDosingPump(device);
    device.evaluateSchedule(mondayAt(9U, 0U), 11U);

    TEST_ASSERT_TRUE(device.handleCommand(customCommand("startDose:500:0")));
    TEST_ASSERT_TRUE(device.runType() == DosingRunType::Calibration);
    device.tick100ms(11U + 5000U);
    TEST_ASSERT_FALSE(device.running());
    TEST_ASSERT_EQUAL_UINT32(500U, device.lastRunDosedCentiMl());
    TEST_ASSERT_EQUAL_UINT32(0U, device.todayDosedCentiMl());
    TEST_ASSERT_EQUAL_UINT32(100000U - 500U, device.containerCurrentCentiMl());
    TEST_ASSERT_EQUAL(0U, journal.records.size());
}

void test_dosing_pump_scheduled_dose_fires_once_within_grace() {
    DosingPumpDeviceConfigV1 config = makeDosingPumpConfig();
    config.doses[0] = DosingPumpDoseV1{8U * 60U, 200U}; // 08:00, 2 ml
    config.doseCount = 1U;
    DosingPumpDevice device(config);
    FakePumpSwitch pump;
    bindDosingPumpIdentity(device, 1U, 2U);
    device.setDependencyRuntime(DeviceRole::Switch, &pump);
    startDosingPump(device);
    TEST_ASSERT_TRUE(device.handleCommand(customCommand("auto")));

    device.evaluateSchedule(mondayAt(7U, 59U), 100U);
    TEST_ASSERT_FALSE(device.running());

    device.evaluateSchedule(mondayAt(8U, 0U, 30U), 200U);
    TEST_ASSERT_TRUE(device.running());
    TEST_ASSERT_TRUE(device.runType() == DosingRunType::Scheduled);

    device.tick100ms(200U + 2000U); // 2 ml @ 1 ml/s
    TEST_ASSERT_FALSE(device.running());
    TEST_ASSERT_EQUAL_UINT32(200U, device.todayDosedCentiMl());

    // Same minute again - the fired bit blocks a re-dose.
    device.evaluateSchedule(mondayAt(8U, 1U), 3000U);
    TEST_ASSERT_FALSE(device.running());

    // Manual mode never auto-fires.
    TEST_ASSERT_TRUE(device.handleCommand(customCommand("manual")));
    device.evaluateSchedule(mondayAt(8U, 2U), 4000U);
    TEST_ASSERT_FALSE(device.running());
}

void test_dosing_pump_missed_doses_are_dropped_not_dosed_late() {
    DosingPumpDeviceConfigV1 config = makeDosingPumpConfig();
    config.doses[0] = DosingPumpDoseV1{8U * 60U, 200U};
    config.doses[1] = DosingPumpDoseV1{12U * 60U, 300U};
    config.doseCount = 2U;
    DosingPumpDevice device(config);
    FakePumpSwitch pump;
    bindDosingPumpIdentity(device, 1U, 2U);
    device.setDependencyRuntime(DeviceRole::Switch, &pump);
    startDosingPump(device);
    TEST_ASSERT_TRUE(device.handleCommand(customCommand("auto")));

    // Boot at 13:00: both doses are far past their grace window - dropped, not fired.
    device.evaluateSchedule(mondayAt(13U, 0U), 100U);
    TEST_ASSERT_FALSE(device.running());
    TEST_ASSERT_EQUAL_UINT32(0U, device.todayDosedCentiMl());

    uint32_t epoch = 0U;
    uint16_t amount = 0U;
    TEST_ASSERT_TRUE(device.nextDoseAt(epoch, amount));
    const uint32_t tuesday = mondayAt(0U, 0U).unixtime() / 86400UL + 1U;
    TEST_ASSERT_EQUAL_UINT32(tuesday * 86400UL + 8UL * 3600UL, epoch);
}

void test_dosing_pump_skip_next_consumes_and_empty_block_drops() {
    DosingPumpDeviceConfigV1 config = makeDosingPumpConfig();
    config.doses[0] = DosingPumpDoseV1{8U * 60U, 200U};
    config.doses[1] = DosingPumpDoseV1{12U * 60U, 300U};
    config.doseCount = 2U;
    DosingPumpDevice device(config);
    FakePumpSwitch pump;
    bindDosingPumpIdentity(device, 1U, 2U);
    device.setDependencyRuntime(DeviceRole::Switch, &pump);
    startDosingPump(device);
    TEST_ASSERT_TRUE(device.handleCommand(customCommand("auto")));

    TEST_ASSERT_TRUE(device.handleCommand(customCommand("skipNext:0:1")));
    TEST_ASSERT_EQUAL_UINT16(0x1U, device.skipNextMask());
    device.evaluateSchedule(mondayAt(8U, 0U), 100U);
    TEST_ASSERT_FALSE(device.running());
    TEST_ASSERT_EQUAL_UINT16(0U, device.skipNextMask()); // consumed

    // Empty container + blockAutoWhenEmpty: the 12:00 dose is dropped without running.
    TEST_ASSERT_TRUE(device.handleCommand(customCommand("setVolume:0")));
    device.evaluateSchedule(mondayAt(12U, 0U), 200U);
    TEST_ASSERT_FALSE(device.running());
    TEST_ASSERT_EQUAL_UINT32(0U, device.todayDosedCentiMl());
    TEST_ASSERT_EQUAL_UINT32(0U, pump.requestCount); // motor never engaged
}

void test_dosing_pump_level_sensor_reports_empty_with_invert() {
    DosingPumpDevice device(makeDosingPumpConfig());
    FakePumpSwitch pump;
    FakeLevelSensor sensor;
    bindDosingPumpIdentity(device, 1U, 2U, 3U, false);
    device.setDependencyRuntime(DeviceRole::Switch, &pump);
    device.setDependencyRuntime(DeviceRole::Condition, &sensor);
    startDosingPump(device);

    TEST_ASSERT_TRUE(device.levelSensorPresent());
    TEST_ASSERT_FALSE(device.containerEmpty());
    sensor.active_ = true;
    TEST_ASSERT_TRUE(device.containerEmpty());

    DosingPumpDevice inverted(makeDosingPumpConfig());
    FakePumpSwitch invertedPump;
    FakeLevelSensor invertedSensor;
    bindDosingPumpIdentity(inverted, 4U, 5U, 6U, true);
    inverted.setDependencyRuntime(DeviceRole::Switch, &invertedPump);
    inverted.setDependencyRuntime(DeviceRole::Condition, &invertedSensor);
    startDosingPump(inverted);
    TEST_ASSERT_TRUE(inverted.containerEmpty()); // inactive + invert = empty
    invertedSensor.active_ = true;
    TEST_ASSERT_FALSE(inverted.containerEmpty());
}

void test_dosing_pump_day_rollover_resets_today_totals() {
    DosingPumpDevice device(makeDosingPumpConfig());
    FakePumpSwitch pump;
    bindDosingPumpIdentity(device, 1U, 2U);
    device.setDependencyRuntime(DeviceRole::Switch, &pump);
    startDosingPump(device);
    device.evaluateSchedule(mondayAt(23U, 0U), 100U);

    TEST_ASSERT_TRUE(device.handleCommand(customCommand("startDose:300:1")));
    device.tick100ms(11U + 3000U);
    TEST_ASSERT_EQUAL_UINT32(300U, device.todayDosedCentiMl());

    device.evaluateSchedule(DateTime(2026U, 7U, 14U, 0U, 1U), 200000U);
    TEST_ASSERT_EQUAL_UINT32(0U, device.todayDosedCentiMl());
    // The container level is consumption state, not a daily counter - it survives the rollover.
    TEST_ASSERT_EQUAL_UINT32(100000U - 300U, device.containerCurrentCentiMl());
}

void test_dosing_pump_retained_state_round_trip() {
    MemoryConfigStorage storage;
    DeviceRetainedDataStore retainedStore(storage);
    TEST_ASSERT_TRUE(retainedStore.begin(false));

    DosingPumpDeviceConfigV1 config = makeDosingPumpConfig();
    config.doses[0] = DosingPumpDoseV1{8U * 60U, 200U};
    config.doses[1] = DosingPumpDoseV1{12U * 60U, 300U};
    config.doseCount = 2U;

    DosingPumpDevice device(config);
    FakePumpSwitch pump;
    bindDosingPumpIdentity(device, 50U, 51U);
    device.setDependencyRuntime(DeviceRole::Switch, &pump);
    startDosingPump(device);

    TEST_ASSERT_TRUE(device.handleCommand(customCommand("auto")));
    TEST_ASSERT_TRUE(device.handleCommand(customCommand("setVolume:25000")));
    TEST_ASSERT_TRUE(device.handleCommand(customCommand("skipNext:1:1")));
    TEST_ASSERT_TRUE(device.retainedStateDirty());
    TEST_ASSERT_TRUE(device.saveRetainedState(retainedStore).ok());

    DosingPumpDevice reloaded(config);
    FakePumpSwitch reloadedPump;
    bindDosingPumpIdentity(reloaded, 50U, 51U);
    reloaded.setDependencyRuntime(DeviceRole::Switch, &reloadedPump);
    TEST_ASSERT_TRUE(reloaded.loadRetainedState(retainedStore).ok());
    TEST_ASSERT_TRUE(reloaded.autoMode());
    TEST_ASSERT_EQUAL_UINT32(25000U, reloaded.containerCurrentCentiMl());
    TEST_ASSERT_EQUAL_UINT16(0x2U, reloaded.skipNextMask());
}

void test_dosing_pump_reboot_mid_run_never_double_doses() {
    DosingPumpDeviceConfigV1 config = makeDosingPumpConfig();
    config.doses[0] = DosingPumpDoseV1{8U * 60U, 6000U}; // 60 ml -> 60 s run
    config.doseCount = 1U;

    MemoryConfigStorage storage;
    DeviceRetainedDataStore retainedStore(storage);
    TEST_ASSERT_TRUE(retainedStore.begin(false));

    DosingPumpDevice device(config);
    FakePumpSwitch pump;
    bindDosingPumpIdentity(device, 60U, 61U);
    device.setDependencyRuntime(DeviceRole::Switch, &pump);
    startDosingPump(device);
    TEST_ASSERT_TRUE(device.handleCommand(customCommand("auto")));

    device.evaluateSchedule(mondayAt(8U, 0U), 100U);
    TEST_ASSERT_TRUE(device.running());
    // The fired bit is persisted at run start - save mid-run, "reboot", reload.
    TEST_ASSERT_TRUE(device.saveRetainedState(retainedStore).ok());

    DosingPumpDevice rebooted(config);
    FakePumpSwitch rebootedPump;
    bindDosingPumpIdentity(rebooted, 60U, 61U);
    rebooted.setDependencyRuntime(DeviceRole::Switch, &rebootedPump);
    TEST_ASSERT_TRUE(rebooted.loadRetainedState(retainedStore).ok());
    startDosingPump(rebooted);

    // Still within the grace window after reboot - but the fired bit blocks a second run.
    rebooted.evaluateSchedule(mondayAt(8U, 2U), 100U);
    TEST_ASSERT_FALSE(rebooted.running());
    TEST_ASSERT_EQUAL_UINT32(0U, rebootedPump.requestCount);
}
