#include "time/TimeZoneTable.h"

#include <ArduinoJson.h>
#include <fstream>
#include <sstream>
#include <unity.h>

using namespace ewfm;

void test_time_zone_table_has_100_entries() {
    TEST_ASSERT_EQUAL(100, static_cast<int>(kTimeZoneTableCount));
}

void test_time_zone_table_lookup_finds_known_zone_with_dst_rule() {
    const TimeZoneEntry* entry = findTimeZoneEntry("America/New_York");
    TEST_ASSERT_NOT_NULL(entry);
    TEST_ASSERT_EQUAL(-240, entry->dst.offsetMinutes);
    TEST_ASSERT_EQUAL(-300, entry->std.offsetMinutes);
    TEST_ASSERT_EQUAL_STRING("EDT", entry->dst.abbrev);
    TEST_ASSERT_EQUAL_STRING("EST", entry->std.abbrev);
}

void test_time_zone_table_lookup_finds_fixed_offset_zone() {
    const TimeZoneEntry* entry = findTimeZoneEntry("Etc/GMT");
    TEST_ASSERT_NOT_NULL(entry);
    TEST_ASSERT_EQUAL(0, entry->dst.offsetMinutes);
    TEST_ASSERT_EQUAL(0, entry->std.offsetMinutes);
}

void test_time_zone_table_lookup_rejects_unknown_and_null() {
    TEST_ASSERT_NULL(findTimeZoneEntry("Not/AZone"));
    TEST_ASSERT_NULL(findTimeZoneEntry(nullptr));
}

// Guards against timezones.json (the source data the table was ported from) silently drifting out
// of sync with the embedded C++ table. Only the id set/order is compared: the firmware table no
// longer stores the human-readable names (those live in the SPA, which is the sole consumer of the
// labels) - see src/time/TimeZoneTable.h.
void test_time_zone_table_matches_timezones_json() {
    std::ifstream file("test/test_core/timezones.json");
    TEST_ASSERT_TRUE_MESSAGE(file.good(), "expected to read test/test_core/timezones.json (native tests run from the repo root)");

    std::ostringstream buffer;
    buffer << file.rdbuf();
    const std::string contents = buffer.str();

    DynamicJsonDocument doc(16384);
    const DeserializationError error = deserializeJson(doc, contents);
    TEST_ASSERT_FALSE_MESSAGE(static_cast<bool>(error), "timezones.json must parse as valid JSON");

    JsonArray entries = doc.as<JsonArray>();
    TEST_ASSERT_EQUAL(static_cast<int>(kTimeZoneTableCount), static_cast<int>(entries.size()));

    size_t index = 0;
    for (JsonObject entry : entries) {
        TEST_ASSERT_TRUE(index < kTimeZoneTableCount);
        TEST_ASSERT_EQUAL_STRING(kTimeZoneTable[index].id, entry["id"].as<const char*>());
        ++index;
    }
}
