#include "metrics/MetricValueFormat.h"

#include <cstring>
#include <unity.h>

using namespace ewfm;

void test_format_date_time_pattern_renders_each_token() {
    const DateTime value(2026, 7, 11, 9, 5, 3); // Saturday
    char output[64]{};

    TEST_ASSERT_TRUE(formatDateTimePattern(value, "YYYY-MM-DD HH:mm:ss", output, sizeof(output)));
    TEST_ASSERT_EQUAL_STRING("2026-07-11 09:05:03", output);

    TEST_ASSERT_TRUE(formatDateTimePattern(value, "YY/M/D H:m:s", output, sizeof(output)));
    TEST_ASSERT_EQUAL_STRING("26/7/11 9:5:3", output);

    TEST_ASSERT_TRUE(formatDateTimePattern(value, "EEEE", output, sizeof(output)));
    TEST_ASSERT_EQUAL_STRING("Saturday", output);

    TEST_ASSERT_TRUE(formatDateTimePattern(value, "EEE", output, sizeof(output)));
    TEST_ASSERT_EQUAL_STRING("Sat", output);
}

void test_format_date_time_pattern_escapes_literal_brackets() {
    const DateTime value(2026, 7, 11, 9, 5, 3);
    char output[64]{};

    TEST_ASSERT_TRUE(formatDateTimePattern(value, "HH:mm:ss [hrs]", output, sizeof(output)));
    TEST_ASSERT_EQUAL_STRING("09:05:03 hrs", output);
}

void test_format_date_time_pattern_reports_overflow() {
    const DateTime value(2026, 7, 11, 9, 5, 3);
    char output[6]{};

    TEST_ASSERT_FALSE(formatDateTimePattern(value, "YYYY-MM-DD", output, sizeof(output)));
}

void test_format_duration_pattern_renders_total_hours_and_escapes() {
    char output[64]{};

    // 26h 3m 4s, expressed in ms - total hours must not wrap at 24.
    const uint32_t durationMs = ((26UL * 3600UL) + (3UL * 60UL) + 4UL) * 1000UL;

    TEST_ASSERT_TRUE(formatDurationPattern(durationMs, "HH:mm:ss", output, sizeof(output)));
    TEST_ASSERT_EQUAL_STRING("26:03:04", output);

    TEST_ASSERT_TRUE(formatDurationPattern(durationMs, "H:m:s [hrs]", output, sizeof(output)));
    TEST_ASSERT_EQUAL_STRING("26:3:4 hrs", output);
}

void test_format_duration_pattern_rejects_date_only_tokens() {
    char output[64]{};
    TEST_ASSERT_FALSE(formatDurationPattern(3723000U, "EEEE", output, sizeof(output)));
    TEST_ASSERT_FALSE(formatDurationPattern(3723000U, "YYYY", output, sizeof(output)));
    TEST_ASSERT_FALSE(formatDurationPattern(3723000U, "MM/DD", output, sizeof(output)));
}

void test_format_duration_default_matches_legacy_uptime_text() {
    char output[32]{};
    formatDurationDefault(3723000U, output, sizeof(output));
    TEST_ASSERT_EQUAL_STRING("1:02:03", output);

    formatDurationDefault(63000U, output, sizeof(output));
    TEST_ASSERT_EQUAL_STRING("1:03", output);
}

void test_format_fixed_decimals_handles_float_and_int_and_duration() {
    char output[32]{};

    MetricValue floatValue{};
    floatValue.valueType = MetricValueType::Float;
    floatValue.number = 21.2345f;
    TEST_ASSERT_TRUE(formatFixedDecimals(floatValue, 2U, output, sizeof(output)));
    TEST_ASSERT_EQUAL_STRING("21.23", output);

    TEST_ASSERT_TRUE(formatFixedDecimals(floatValue, 0U, output, sizeof(output)));
    TEST_ASSERT_EQUAL_STRING("21", output);

    MetricValue intValue{};
    intValue.valueType = MetricValueType::Int;
    intValue.number = static_cast<int32_t>(7);
    TEST_ASSERT_TRUE(formatFixedDecimals(intValue, 1U, output, sizeof(output)));
    TEST_ASSERT_EQUAL_STRING("7.0", output);

    MetricValue durationValue{};
    durationValue.valueType = MetricValueType::Duration;
    durationValue.number = static_cast<int32_t>(1500);
    TEST_ASSERT_TRUE(formatFixedDecimals(durationValue, 1U, output, sizeof(output)));
    TEST_ASSERT_EQUAL_STRING("1500.0", output);
}

void test_format_fixed_decimals_rejects_non_numeric_and_out_of_range_digits() {
    char output[32]{};

    MetricValue stringValue{};
    stringValue.valueType = MetricValueType::String;
    TEST_ASSERT_FALSE(formatFixedDecimals(stringValue, 2U, output, sizeof(output)));

    MetricValue floatValue{};
    floatValue.valueType = MetricValueType::Float;
    floatValue.number = 1.0f;
    TEST_ASSERT_FALSE(formatFixedDecimals(floatValue, 7U, output, sizeof(output)));
}

void test_metric_value_numeric_preview_widens_active_alternative() {
    MetricValue dateTimeValue{};
    dateTimeValue.valueType = MetricValueType::DateTime;
    const DateTime fixedTime(2026, 7, 11, 9, 5, 3);
    dateTimeValue.number = fixedTime;
    TEST_ASSERT_TRUE(metricValueHasNumericPreview(dateTimeValue));
    TEST_ASSERT_EQUAL_UINT32(fixedTime.unixtime(), static_cast<uint32_t>(metricValueNumericPreview(dateTimeValue)));

    MetricValue stringValue{};
    stringValue.valueType = MetricValueType::String;
    TEST_ASSERT_FALSE(metricValueHasNumericPreview(stringValue));
}
