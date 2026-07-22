#include "devices/analog/scheduled/ScheduledAnalogOutputDeviceConfig.h"
#include "devices/analog/scheduled/presets/SchedulePreset.h"

#include <ArduinoJson.h>
#include <cstring>
#include <unity.h>

using namespace ewfm;

namespace {
void markAllDeleted(ScheduledAnalogOutputPointV1 (&points)[kMaxScheduledAnalogOutputPoints]) {
    for (ScheduledAnalogOutputPointV1& point : points) {
        point.deleted = 1U;
    }
}
} // namespace

void test_schedule_preset_record_stamps_magic_and_truncates_name() {
    ScheduledAnalogOutputPointV1 points[kMaxScheduledAnalogOutputPoints]{};
    markAllDeleted(points);
    points[0] = {0U, 480U, 1000U};
    points[1] = {0U, 1200U, 3000U};

    SchedulePresetRecordV1 record{};
    TEST_ASSERT_FALSE(schedulePresetRecordValid(record)); // freshly zeroed: no magic

    const char* longName = "0123456789012345678901234567890123456789"; // 40 chars, must truncate to 32
    buildSchedulePresetRecord(record, longName, points);

    TEST_ASSERT_TRUE(schedulePresetRecordValid(record));
    TEST_ASSERT_EQUAL_UINT(kMaxSchedulePresetNameLength, std::strlen(record.name));
    TEST_ASSERT_EQUAL_UINT16(480U, record.points[0].minuteOfDay);
    TEST_ASSERT_EQUAL_UINT16(3000U, record.points[1].state);
    TEST_ASSERT_EQUAL_UINT8(0U, record.points[0].deleted);
    TEST_ASSERT_EQUAL_UINT8(1U, record.points[2].deleted);
}

void test_schedule_preset_record_round_trips_as_raw_block() {
    ScheduledAnalogOutputPointV1 points[kMaxScheduledAnalogOutputPoints]{};
    markAllDeleted(points);
    points[0] = {0U, 60U, 42U};

    SchedulePresetRecordV1 original{};
    buildSchedulePresetRecord(original, "reef", points);

    // Trivially-copyable: a byte copy reproduces the record exactly, as the LittleFS storage relies on.
    SchedulePresetRecordV1 copy{};
    std::memcpy(&copy, &original, sizeof(copy));
    TEST_ASSERT_TRUE(schedulePresetRecordValid(copy));
    TEST_ASSERT_EQUAL_STRING("reef", copy.name);
    TEST_ASSERT_EQUAL_UINT16(60U, copy.points[0].minuteOfDay);
    TEST_ASSERT_EQUAL_UINT16(42U, copy.points[0].state);
}

void test_parse_and_validate_analog_schedule_points_shared_helpers() {
    StaticJsonDocument<512> doc;
    JsonArray arr = doc.to<JsonArray>();
    JsonObject a = arr.createNestedObject();
    a["minuteOfDay"] = 480;
    a["state"] = 50; // JSON state is a percentage; the codec scales it to a raw analog level
    JsonObject b = arr.createNestedObject();
    b["minuteOfDay"] = 1200;
    b["state"] = 90;

    ScheduledAnalogOutputPointV1 points[kMaxScheduledAnalogOutputPoints]{};
    markAllDeleted(points);
    const char* error = nullptr;
    JsonArrayConst constArr = arr;
    TEST_ASSERT_TRUE(parseAnalogSchedulePoints(constArr, points, error));
    TEST_ASSERT_TRUE(validateAnalogSchedulePoints(points).ok());
    TEST_ASSERT_EQUAL_UINT16(480U, points[0].minuteOfDay);
    TEST_ASSERT_EQUAL_UINT16(percentToAnalogOutputState(50U), points[0].state);
    TEST_ASSERT_EQUAL_UINT16(percentToAnalogOutputState(90U), points[1].state);

    ScheduledAnalogOutputPointV1 duplicate[kMaxScheduledAnalogOutputPoints]{};
    markAllDeleted(duplicate);
    duplicate[0] = {0U, 100U, 10U};
    duplicate[1] = {0U, 100U, 20U};
    TEST_ASSERT_FALSE(validateAnalogSchedulePoints(duplicate).ok()); // duplicate active times

    ScheduledAnalogOutputPointV1 outOfRange[kMaxScheduledAnalogOutputPoints]{};
    markAllDeleted(outOfRange);
    outOfRange[0] = {0U, 100U, static_cast<uint16_t>(kAnalogOutputLevelMax + 1U)};
    TEST_ASSERT_FALSE(validateAnalogSchedulePoints(outOfRange).ok());

    ScheduledAnalogOutputPointV1 empty[kMaxScheduledAnalogOutputPoints]{};
    markAllDeleted(empty);
    TEST_ASSERT_FALSE(validateAnalogSchedulePoints(empty).ok()); // no active points
}
