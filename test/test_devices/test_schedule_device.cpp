#include "devices/core/DeviceTypes.h"
#include "devices/schedule/ScheduleDevice.h"
#include "devices/schedule/ScheduleDeviceConfig.h"
#include "integrations/rest/schedule/ScheduleDeviceApiAdapter.h"
#include "time/DateTime.h"

#include <cstdio>
#include <unity.h>

using namespace ewfm;

namespace {

constexpr uint8_t kSunday = 0;
constexpr uint8_t kMonday = 1;
constexpr uint8_t kFriday = 5;
constexpr uint8_t kSaturday = 6;

// 1970-01-04 is a Sunday, so 1970-01-(4+weekdayIndex) lands exactly on the requested weekday
// (0=Sunday..6=Saturday) - lets tests build a concrete DateTime instead of passing raw
// weekday/minute parameters around, matching how ScheduleDevice itself takes a DateTime.
DateTime dateTimeAt(uint8_t weekdayIndex, uint16_t minuteOfDay) {
    return DateTime(1970, 1, static_cast<uint8_t>(4U + weekdayIndex), static_cast<uint8_t>(minuteOfDay / 60U),
                    static_cast<uint8_t>(minuteOfDay % 60U), 0U);
}

ScheduleDeviceConfigV1 makeScheduleConfig() {
    ScheduleDeviceConfigV1 config{};
    config.enabled = true;
    std::snprintf(config.name, sizeof(config.name), "%s", "schedule");
    config.ruleCount = 0U;
    return config;
}

ScheduleRuleV1 makeAlwaysOnRule(uint8_t weekDays, uint16_t startMinuteOfDay, uint16_t endMinuteOfDay) {
    ScheduleRuleV1 rule{};
    rule.enabled = 1U;
    rule.weekDays = weekDays;
    rule.startMinuteOfDay = startMinuteOfDay;
    rule.endMinuteOfDay = endMinuteOfDay;
    rule.mode = static_cast<uint8_t>(ScheduleRuleMode::AlwaysOn);
    return rule;
}

ScheduleRuleV1 makeIntervalRule(uint8_t weekDays, uint16_t startMinuteOfDay, uint16_t endMinuteOfDay, uint16_t intervalsPerWindow,
                                uint16_t durationMinutes) {
    ScheduleRuleV1 rule{};
    rule.enabled = 1U;
    rule.weekDays = weekDays;
    rule.startMinuteOfDay = startMinuteOfDay;
    rule.endMinuteOfDay = endMinuteOfDay;
    rule.mode = static_cast<uint8_t>(ScheduleRuleMode::Interval);
    rule.intervalsPerWindow = intervalsPerWindow;
    rule.durationMinutes = durationMinutes;
    return rule;
}

} // namespace

void test_schedule_config_codec_round_trip_and_validation() {
    ScheduleDeviceConfigV1 config = makeScheduleConfig();
    config.rules[0] = makeAlwaysOnRule(0x7F, 8U * 60U, 20U * 60U);
    config.ruleCount = 1U;

    uint8_t buffer[kMaxDeviceConfigBytes]{};
    const size_t size = scheduleDeviceConfigSize(config);
    TEST_ASSERT_TRUE(encodeFixedConfigBlob(ScheduleDeviceConfigV1::kMagic, config, buffer, size));

    ScheduleDeviceConfigV1 decoded{};
    TEST_ASSERT_TRUE(decodeValidatedFixedConfigBlob(ScheduleDeviceConfigV1::kMagic, buffer, size, decoded));
    TEST_ASSERT_EQUAL_UINT8(1U, decoded.ruleCount);
    TEST_ASSERT_EQUAL_UINT8(0x7F, decoded.rules[0].weekDays);
    TEST_ASSERT_EQUAL_UINT16(8U * 60U, decoded.rules[0].startMinuteOfDay);
    TEST_ASSERT_EQUAL_UINT16(20U * 60U, decoded.rules[0].endMinuteOfDay);
    TEST_ASSERT_TRUE((decoded).validate().ok());
}

void test_schedule_config_rejects_duration_exceeding_interval_slice() {
    ScheduleDeviceConfigV1 config = makeScheduleConfig();
    // Window is 60 minutes split into 4 slices of 15 minutes each; a 20-minute duration cannot fit
    // in one slice.
    config.rules[0] = makeIntervalRule(0x7F, 0U, 60U, 4U, 20U);
    config.ruleCount = 1U;
    TEST_ASSERT_FALSE((config).validate().ok());
}

void test_schedule_type_and_api_adapter_are_registered() {
    DeviceTypeRegistry types = DeviceTypeRegistry::withDefaults();
    const DeviceTypeDescriptor* descriptor = types.find(kScheduleDeviceTypeId);
    TEST_ASSERT_NOT_NULL(descriptor);
    TEST_ASSERT_EQUAL_STRING("ScheduleDevice", descriptor->name);
    TEST_ASSERT_TRUE(descriptor->providedRoles.contains(DeviceRole::Schedule));
    TEST_ASSERT_TRUE(descriptor->providedRoles.contains(DeviceRole::Condition));
    TEST_ASSERT_TRUE(descriptor->dependencyRequirements.empty());

    DeviceApiAdapterRegistry adapters = DeviceApiAdapterRegistry::withDefaults();
    const IDeviceApiAdapter* adapter = adapters.find(kScheduleDeviceTypeId);
    TEST_ASSERT_NOT_NULL(adapter);
    TEST_ASSERT_EQUAL_STRING("schedule", adapter->typeName());
}

// isActive()/timeValid() read DateTime::current() (Arduino-only, see DateTimeTimezone.cpp), so on
// native they always report "no clock available" - that's the correct, safe default (never treat
// an unset/unsynced clock as if it were plausible). The rule-evaluation math itself is exercised
// below via isActiveAt(), which takes an explicit DateTime and has no wall-clock dependency at all.
void test_schedule_is_inactive_without_a_real_clock_on_native() {
    ScheduleDeviceConfigV1 config = makeScheduleConfig();
    config.rules[0] = makeAlwaysOnRule(0x7F, 0U, kScheduleMinutesPerDay - 1U);
    config.ruleCount = 1U;
    ScheduleDevice device(config);

    TEST_ASSERT_FALSE(device.isActive());
    TEST_ASSERT_FALSE(device.timeValid());
    TEST_ASSERT_TRUE(device.isActiveAt(dateTimeAt(kMonday, 12U * 60U)));
}

void test_schedule_always_on_rule_respects_weekday_and_window_including_midnight_wrap() {
    ScheduleDeviceConfigV1 weekdayConfig = makeScheduleConfig();
    // Monday-Friday, 08:00-20:00.
    weekdayConfig.rules[0] = makeAlwaysOnRule((1U << kMonday) | (1U << 2) | (1U << 3) | (1U << 4) | (1U << kFriday), 8U * 60U, 20U * 60U);
    weekdayConfig.ruleCount = 1U;
    ScheduleDevice weekdayDevice(weekdayConfig);

    TEST_ASSERT_TRUE(weekdayDevice.isActiveAt(dateTimeAt(kMonday, 8U * 60U)));
    TEST_ASSERT_TRUE(weekdayDevice.isActiveAt(dateTimeAt(kMonday, (20U * 60U) - 1U)));
    TEST_ASSERT_FALSE(weekdayDevice.isActiveAt(dateTimeAt(kMonday, 20U * 60U)));
    TEST_ASSERT_FALSE(weekdayDevice.isActiveAt(dateTimeAt(kMonday, (8U * 60U) - 1U)));
    TEST_ASSERT_FALSE(weekdayDevice.isActiveAt(dateTimeAt(kSaturday, 12U * 60U)));

    ScheduleDeviceConfigV1 overnightConfig = makeScheduleConfig();
    // Every day, 22:00-06:00 (wraps past midnight).
    overnightConfig.rules[0] = makeAlwaysOnRule(0x7F, 22U * 60U, 6U * 60U);
    overnightConfig.ruleCount = 1U;
    ScheduleDevice overnightDevice(overnightConfig);

    TEST_ASSERT_TRUE(overnightDevice.isActiveAt(dateTimeAt(kMonday, 23U * 60U)));
    TEST_ASSERT_TRUE(overnightDevice.isActiveAt(dateTimeAt(kMonday, 1U * 60U)));
    TEST_ASSERT_FALSE(overnightDevice.isActiveAt(dateTimeAt(kMonday, 12U * 60U)));
}

void test_schedule_interval_rule_is_duty_cycle_not_edge_triggered() {
    ScheduleDeviceConfigV1 config = makeScheduleConfig();
    // Whole day (start==end), split into 4 six-hour slices, 30 minutes on at the start of each.
    config.rules[0] = makeIntervalRule(0x7F, 0U, 0U, 4U, 30U);
    config.ruleCount = 1U;
    ScheduleDevice device(config);

    // First slice: [00:00, 06:00), on for the first 30 minutes.
    TEST_ASSERT_TRUE(device.isActiveAt(dateTimeAt(kMonday, 0U)));
    TEST_ASSERT_TRUE(device.isActiveAt(dateTimeAt(kMonday, 29U)));
    TEST_ASSERT_FALSE(device.isActiveAt(dateTimeAt(kMonday, 30U)));
    TEST_ASSERT_FALSE(device.isActiveAt(dateTimeAt(kMonday, 3U * 60U)));

    // Second slice starts at 06:00 - on again for its first 30 minutes, stateless (no memory of the
    // first slice's pulse).
    TEST_ASSERT_TRUE(device.isActiveAt(dateTimeAt(kMonday, 6U * 60U)));
    TEST_ASSERT_FALSE(device.isActiveAt(dateTimeAt(kMonday, 6U * 60U + 30U)));
}

// statusRuntime() is the capability a Condition-role dependent (e.g. AutoSwitchDevice) reads -
// confirm it's non-null and delegates to the exact same cached isActive() as IScheduleRuntime.
void test_schedule_status_runtime_delegates_to_is_active() {
    ScheduleDeviceConfigV1 config = makeScheduleConfig();
    config.rules[0] = makeAlwaysOnRule(0x7F, 0U, kScheduleMinutesPerDay - 1U);
    config.ruleCount = 1U;
    ScheduleDevice device(config);

    const IStatusRuntime* status = device.statusRuntime();
    TEST_ASSERT_NOT_NULL(status);
    TEST_ASSERT_EQUAL(device.isActive(), status->isActive());
}

void test_schedule_multiple_rules_combine_with_or() {
    ScheduleDeviceConfigV1 config = makeScheduleConfig();
    // Rule 0: weekdays 08:00-20:00. Rule 1: weekends 10:00-14:00.
    config.rules[0] = makeAlwaysOnRule((1U << kMonday) | (1U << 2) | (1U << 3) | (1U << 4) | (1U << kFriday), 8U * 60U, 20U * 60U);
    config.rules[1] = makeAlwaysOnRule((1U << kSunday) | (1U << kSaturday), 10U * 60U, 14U * 60U);
    config.ruleCount = 2U;
    ScheduleDevice device(config);

    TEST_ASSERT_TRUE(device.isActiveAt(dateTimeAt(kMonday, 9U * 60U)));
    TEST_ASSERT_TRUE(device.isActiveAt(dateTimeAt(kSaturday, 11U * 60U)));
    TEST_ASSERT_FALSE(device.isActiveAt(dateTimeAt(kSaturday, 20U * 60U)));
}
