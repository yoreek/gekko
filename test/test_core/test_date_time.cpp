#include "time/DateTime.h"
#include "time/Iso8601.h"
#include "time/TimeDelta.h"
#include "time/TimeZoneInfo.h"

#include <cstring>
#include <string>
#include <unity.h>

using namespace ewfm;

void test_date_time_epoch_zero_is_1970_01_01_thursday() {
    const DateTime dt(0U);
    TEST_ASSERT_EQUAL_UINT16(1970, dt.year());
    TEST_ASSERT_EQUAL_UINT8(1, dt.month());
    TEST_ASSERT_EQUAL_UINT8(1, dt.day());
    TEST_ASSERT_EQUAL_UINT8(0, dt.hour());
    TEST_ASSERT_EQUAL_UINT8(5, dt.weekday()); // Sunday is day 1 -> Thursday is day 5
}

void test_date_time_known_y2k_epoch_anchor() {
    const DateTime dt(2000, 1, 1);
    TEST_ASSERT_EQUAL_UINT32(946684800UL, dt.unixtime());
}

void test_date_time_field_round_trip_through_epoch() {
    const DateTime original(2024, 3, 5, 12, 45, 55);
    const DateTime rebuilt(original.unixtime());
    TEST_ASSERT_EQUAL_UINT16(2024, rebuilt.year());
    TEST_ASSERT_EQUAL_UINT8(3, rebuilt.month());
    TEST_ASSERT_EQUAL_UINT8(5, rebuilt.day());
    TEST_ASSERT_EQUAL_UINT8(12, rebuilt.hour());
    TEST_ASSERT_EQUAL_UINT8(45, rebuilt.minute());
    TEST_ASSERT_EQUAL_UINT8(55, rebuilt.second());
}

void test_date_time_leap_year_day_rolls_into_march() {
    const DateTime lastDayOfFeb(2024, 2, 29);
    const DateTime firstDayOfMarch(2024, 3, 1);
    TEST_ASSERT_EQUAL_UINT32(lastDayOfFeb.unixtime() + 86400UL, firstDayOfMarch.unixtime());
}

void test_date_time_utc_offset_arithmetic() {
    const TimeZoneInfo tz("EST", TimeDelta(0, -5, 0, 0));
    const DateTime local(2024, 3, 5, 7, 0, 0, tz); // 07:00 local == 12:00 UTC at -05:00
    TEST_ASSERT_EQUAL_UINT32(DateTime(2024, 3, 5, 12, 0, 0).unixtime(), local.utcUnixtime());
    TEST_ASSERT_EQUAL_UINT32(local.utcUnixtime(), local.toUtc().unixtime());
}

void test_date_time_comparison_operators_use_utc_instant() {
    const TimeZoneInfo utcPlus2("+2", TimeDelta(0, 2, 0, 0));
    const DateTime earlier(2024, 1, 1, 0, 0, 0);
    const DateTime laterLocal(2024, 1, 1, 3, 0, 0, utcPlus2); // 01:00 UTC, still after `earlier`
    TEST_ASSERT_TRUE(earlier < laterLocal);
    TEST_ASSERT_TRUE(laterLocal > earlier);
    TEST_ASSERT_FALSE(earlier == laterLocal);
}

void test_date_time_delta_addition_and_subtraction() {
    const DateTime start(2024, 1, 1, 0, 0, 0);
    const TimeDelta oneDay(1, 0, 0, 0);
    const DateTime next = start + oneDay;
    TEST_ASSERT_EQUAL_UINT8(2, next.day());
    TEST_ASSERT_TRUE((next - start) == oneDay);
    TEST_ASSERT_TRUE((next - oneDay) == start);
}

void test_time_delta_total_seconds_and_components() {
    const TimeDelta delta(1, 2, 30, 15);
    TEST_ASSERT_EQUAL_INT32(86400 + 2 * 3600 + 30 * 60 + 15, delta.totalSeconds());
    TEST_ASSERT_EQUAL_INT16(1, delta.days());
    TEST_ASSERT_EQUAL_INT8(2, delta.hours());
    TEST_ASSERT_EQUAL_INT8(30, delta.minutes());
    TEST_ASSERT_EQUAL_INT8(15, delta.seconds());
}

void test_time_delta_negative_seconds_round_trip() {
    const TimeDelta delta(-12345);
    const TimeDelta rebuilt(delta.days(), delta.hours(), delta.minutes(), delta.seconds());
    TEST_ASSERT_EQUAL_INT32(-12345, rebuilt.totalSeconds());
}

void test_time_zone_info_named_offset_reports_seconds() {
    const TimeZoneInfo tz("EST", TimeDelta(0, -5, 0, 0));
    TEST_ASSERT_EQUAL_STRING("EST", tz.name());
    TEST_ASSERT_EQUAL_INT32(-18000, tz.offsetSeconds());
}

void test_time_zone_info_auto_formats_utc_offset_name() {
    const TimeZoneInfo tz(TimeDelta(0, 2, 0, 0));
    TEST_ASSERT_EQUAL_STRING("UTC+02:00", tz.name());
}

void test_iso8601_parses_zulu_suffix() {
    const std::string input = "2024-03-05T12:45:55Z";
    const std::optional<DateTime> parsed = parseIso8601(input.c_str(), input.size());
    TEST_ASSERT_TRUE(parsed.has_value());
    TEST_ASSERT_EQUAL_UINT16(2024, parsed->year());
    TEST_ASSERT_EQUAL_UINT8(3, parsed->month());
    TEST_ASSERT_EQUAL_UINT8(5, parsed->day());
    TEST_ASSERT_EQUAL_UINT8(12, parsed->hour());
    TEST_ASSERT_EQUAL_UINT8(45, parsed->minute());
    TEST_ASSERT_EQUAL_UINT8(55, parsed->second());
    TEST_ASSERT_EQUAL_INT32(0, parsed->tzInfo().offsetSeconds());
}

void test_iso8601_parses_explicit_offset() {
    const std::string input = "2024-03-05T12:45:55+02:00";
    const std::optional<DateTime> parsed = parseIso8601(input.c_str(), input.size());
    TEST_ASSERT_TRUE(parsed.has_value());
    TEST_ASSERT_EQUAL_INT32(7200, parsed->tzInfo().offsetSeconds());
}

void test_iso8601_parses_without_seconds() {
    const std::string input = "2024-03-05T12:45";
    const std::optional<DateTime> parsed = parseIso8601(input.c_str(), input.size());
    TEST_ASSERT_TRUE(parsed.has_value());
    TEST_ASSERT_EQUAL_UINT8(0, parsed->second());
}

void test_iso8601_rejects_malformed_input() {
    TEST_ASSERT_FALSE(parseIso8601("not-a-date", 10).has_value());
    const std::string badMonth = "2024-13-05T12:45:55Z";
    TEST_ASSERT_FALSE(parseIso8601(badMonth.c_str(), badMonth.size()).has_value());
    const std::string trailingGarbage = "2024-03-05T12:45:55Zjunk";
    TEST_ASSERT_FALSE(parseIso8601(trailingGarbage.c_str(), trailingGarbage.size()).has_value());
}

void test_iso8601_format_round_trip() {
    char buf[32];

    const DateTime utc(2024, 3, 5, 12, 45, 55);
    formatIso8601(utc, buf, sizeof(buf));
    TEST_ASSERT_EQUAL_STRING("2024-03-05T12:45:55Z", buf);

    const TimeZoneInfo offsetTz("+2", TimeDelta(0, 2, 0, 0));
    const DateTime withOffset(2024, 3, 5, 12, 45, 55, offsetTz);
    formatIso8601(withOffset, buf, sizeof(buf));
    TEST_ASSERT_EQUAL_STRING("2024-03-05T12:45:55+02:00", buf);
}

// ---------------------------------------------------------------------------
// DateTime boundary cases: leap years, month/year rollovers, midnight, weekday
// ---------------------------------------------------------------------------

void test_date_time_leap_year_rules_century_boundaries() {
    // 2000 is divisible by 400 -> leap; Feb has 29 days.
    TEST_ASSERT_EQUAL_UINT32(DateTime(2000, 2, 28).unixtime() + 86400UL, DateTime(2000, 2, 29).unixtime());
    TEST_ASSERT_EQUAL_UINT32(DateTime(2000, 2, 29).unixtime() + 86400UL, DateTime(2000, 3, 1).unixtime());

    // 2100 is divisible by 100 but not 400 -> not leap; Feb has 28 days.
    TEST_ASSERT_EQUAL_UINT32(DateTime(2100, 2, 28).unixtime() + 86400UL, DateTime(2100, 3, 1).unixtime());

    // 2400 is divisible by 400 -> leap again.
    TEST_ASSERT_EQUAL_UINT32(DateTime(2400, 2, 28).unixtime() + 86400UL, DateTime(2400, 2, 29).unixtime());
}

void test_date_time_month_end_rollovers() {
    TEST_ASSERT_EQUAL_UINT32(DateTime(2024, 1, 31).unixtime() + 86400UL, DateTime(2024, 2, 1).unixtime());
    TEST_ASSERT_EQUAL_UINT32(DateTime(2024, 4, 30).unixtime() + 86400UL, DateTime(2024, 5, 1).unixtime());
    TEST_ASSERT_EQUAL_UINT32(DateTime(2024, 12, 31).unixtime() + 86400UL, DateTime(2025, 1, 1).unixtime());
}

void test_date_time_midnight_and_end_of_day_boundaries() {
    const DateTime midnight(2024, 3, 5, 0, 0, 0);
    TEST_ASSERT_EQUAL_UINT32(0, midnight.secondsOfDay());
    TEST_ASSERT_EQUAL_UINT16(0, midnight.minutesOfDay());

    const DateTime lastSecondOfDay(2024, 3, 5, 23, 59, 59);
    TEST_ASSERT_EQUAL_UINT32(86399UL, lastSecondOfDay.secondsOfDay());
    TEST_ASSERT_EQUAL_UINT16(1439, lastSecondOfDay.minutesOfDay());

    // One more second must roll into the next calendar day.
    const DateTime rolledOver(lastSecondOfDay.unixtime() + 1);
    TEST_ASSERT_EQUAL_UINT8(6, rolledOver.day());
    TEST_ASSERT_EQUAL_UINT8(0, rolledOver.hour());
    TEST_ASSERT_EQUAL_UINT8(0, rolledOver.minute());
    TEST_ASSERT_EQUAL_UINT8(0, rolledOver.second());
}

void test_date_time_known_weekday_reference_y2k() {
    // January 1, 2000 was a Saturday (day 7, Sunday = 1).
    TEST_ASSERT_EQUAL_UINT8(7, DateTime(2000, 1, 1).weekday());
}

void test_date_time_equality_across_different_timezone_representations() {
    const DateTime utcNoon(2024, 1, 1, 12, 0, 0);
    const TimeZoneInfo utcPlus3("+3", TimeDelta(0, 3, 0, 0));
    const DateTime localAfternoon(2024, 1, 1, 15, 0, 0, utcPlus3); // 15:00 at +03:00 == 12:00 UTC
    TEST_ASSERT_TRUE(utcNoon == localAfternoon);
    TEST_ASSERT_FALSE(utcNoon != localAfternoon);
}

void test_date_time_ordering_operators_are_consistent() {
    const DateTime a(2024, 1, 1, 0, 0, 0);
    const DateTime b(2024, 1, 1, 0, 0, 1);
    const DateTime c(2024, 1, 1, 0, 0, 1); // same instant as b

    TEST_ASSERT_TRUE(a < b);
    TEST_ASSERT_TRUE(a <= b);
    TEST_ASSERT_TRUE(b > a);
    TEST_ASSERT_TRUE(b >= a);
    TEST_ASSERT_TRUE(b <= c);
    TEST_ASSERT_TRUE(b >= c);
    TEST_ASSERT_FALSE(b < c);
    TEST_ASSERT_FALSE(b > c);
}

// ---------------------------------------------------------------------------
// TimeDelta boundary cases: exact day multiples, +/-1 second around them
// ---------------------------------------------------------------------------

void test_time_delta_zero_has_zero_components() {
    const TimeDelta zero(0);
    TEST_ASSERT_EQUAL_INT16(0, zero.days());
    TEST_ASSERT_EQUAL_INT8(0, zero.hours());
    TEST_ASSERT_EQUAL_INT8(0, zero.minutes());
    TEST_ASSERT_EQUAL_INT8(0, zero.seconds());
}

void test_time_delta_exact_day_multiples() {
    const TimeDelta plusOneDay(86400);
    TEST_ASSERT_EQUAL_INT16(1, plusOneDay.days());
    TEST_ASSERT_EQUAL_INT8(0, plusOneDay.hours());
    TEST_ASSERT_EQUAL_INT8(0, plusOneDay.minutes());
    TEST_ASSERT_EQUAL_INT8(0, plusOneDay.seconds());

    const TimeDelta minusOneDay(-86400);
    TEST_ASSERT_EQUAL_INT16(-1, minusOneDay.days());
    TEST_ASSERT_EQUAL_INT8(0, minusOneDay.hours());
    TEST_ASSERT_EQUAL_INT8(0, minusOneDay.minutes());
    TEST_ASSERT_EQUAL_INT8(0, minusOneDay.seconds());
}

void test_time_delta_one_second_around_day_boundaries() {
    const TimeDelta oneSecondBeforeDay(86399);
    TEST_ASSERT_EQUAL_INT16(0, oneSecondBeforeDay.days());
    TEST_ASSERT_EQUAL_INT8(23, oneSecondBeforeDay.hours());
    TEST_ASSERT_EQUAL_INT8(59, oneSecondBeforeDay.minutes());
    TEST_ASSERT_EQUAL_INT8(59, oneSecondBeforeDay.seconds());

    const TimeDelta oneSecondAfterDay(86401);
    TEST_ASSERT_EQUAL_INT16(1, oneSecondAfterDay.days());
    TEST_ASSERT_EQUAL_INT8(0, oneSecondAfterDay.hours());
    TEST_ASSERT_EQUAL_INT8(0, oneSecondAfterDay.minutes());
    TEST_ASSERT_EQUAL_INT8(1, oneSecondAfterDay.seconds());
}

void test_time_delta_addition_carries_across_day_boundary() {
    const TimeDelta oneSecondBeforeDay(86399);
    const TimeDelta oneSecond(1);
    const TimeDelta oneDay(86400);
    TEST_ASSERT_TRUE((oneSecondBeforeDay + oneSecond) == oneDay);
    TEST_ASSERT_TRUE((oneDay - oneSecond) == oneSecondBeforeDay);
}

void test_time_delta_round_trips_across_curated_boundary_values() {
    const int32_t values[] = {
        0, 1, -1, 59, -59, 60, -60, 3599, -3599, 3600, -3600, 86399, -86399, 86400, -86400, 86401, -86401, 172799, -172799,
    };
    for (const int32_t seconds : values) {
        const TimeDelta original(seconds);
        const TimeDelta rebuilt(original.days(), original.hours(), original.minutes(), original.seconds());
        TEST_ASSERT_EQUAL_INT32(seconds, rebuilt.totalSeconds());
    }
}

// ---------------------------------------------------------------------------
// TimeZoneInfo boundary cases: zero offset sign, half-hour/large offsets, name
// truncation safety
// ---------------------------------------------------------------------------

void test_time_zone_info_zero_offset_uses_positive_sign_not_negative_zero() {
    const TimeZoneInfo tz(TimeDelta(0));
    TEST_ASSERT_EQUAL_STRING("UTC+00:00", tz.name());
}

void test_time_zone_info_formats_negative_half_hour_offset() {
    const TimeZoneInfo tz(TimeDelta(0, -3, -30, 0));
    TEST_ASSERT_EQUAL_STRING("UTC-03:30", tz.name());
    TEST_ASSERT_EQUAL_INT32(-12600, tz.offsetSeconds());
}

void test_time_zone_info_formats_large_positive_offset() {
    const TimeZoneInfo tz(TimeDelta(0, 13, 0, 0)); // Tonga-style +13:00
    TEST_ASSERT_EQUAL_STRING("UTC+13:00", tz.name());
}

void test_time_zone_info_name_truncates_safely_when_too_long() {
    const TimeZoneInfo tz("ABCDEFGHIJKLMNOP", TimeDelta(0));
    // name_ is a fixed char[10] buffer (9 chars + null terminator) - must truncate, never overflow.
    TEST_ASSERT_EQUAL(9, static_cast<int>(strlen(tz.name())));
    TEST_ASSERT_EQUAL_STRING("ABCDEFGHI", tz.name());
}
