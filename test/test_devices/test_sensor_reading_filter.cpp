#include "devices/sensors/filter/SensorFilterConfig.h"
#include "devices/sensors/filter/SensorReadingFilter.h"

#include <ArduinoJson.h>
#include <limits>
#include <unity.h>

using namespace ewfm;

void test_sensor_filter_config_default_is_valid_no_op() {
    SensorFilterConfigV1 config{};
    TEST_ASSERT_TRUE_MESSAGE(config.validate().ok(), config.validate().message);
    TEST_ASSERT_EQUAL_FLOAT(1.0F, config.smoothingWeight);
    TEST_ASSERT_EQUAL_FLOAT(1.0F, config.calibrationFactor);
    TEST_ASSERT_EQUAL_FLOAT(0.0F, config.calibrationOffset);
}

void test_sensor_filter_config_rejects_invalid_coefficients() {
    SensorFilterConfigV1 zeroWeight{};
    zeroWeight.smoothingWeight = 0.0F;
    TEST_ASSERT_FALSE(zeroWeight.validate().ok());

    SensorFilterConfigV1 overWeight{};
    overWeight.smoothingWeight = 1.5F;
    TEST_ASSERT_FALSE(overWeight.validate().ok());

    SensorFilterConfigV1 zeroFactor{};
    zeroFactor.calibrationFactor = 0.0F;
    TEST_ASSERT_FALSE(zeroFactor.validate().ok());

    SensorFilterConfigV1 nanOffset{};
    nanOffset.calibrationOffset = std::numeric_limits<float>::quiet_NaN();
    TEST_ASSERT_FALSE(nanOffset.validate().ok());
}

void test_sensor_filter_config_json_round_trip() {
    SensorFilterConfigV1 config{};
    config.smoothingWeight = 0.25F;
    config.calibrationFactor = 1.02F;
    config.calibrationOffset = -1.5F;

    StaticJsonDocument<128> doc;
    JsonObject json = doc.to<JsonObject>();
    config.writeJson(json);

    SensorFilterConfigV1 parsed{};
    const char* error = nullptr;
    TEST_ASSERT_TRUE_MESSAGE(parsed.parseJson(json, error), error);
    TEST_ASSERT_EQUAL_FLOAT(0.25F, parsed.smoothingWeight);
    TEST_ASSERT_EQUAL_FLOAT(1.02F, parsed.calibrationFactor);
    TEST_ASSERT_EQUAL_FLOAT(-1.5F, parsed.calibrationOffset);

    json["smoothingWeight"] = 0.0F;
    const char* rejectError = nullptr;
    TEST_ASSERT_FALSE(parsed.parseJson(json, rejectError));
    TEST_ASSERT_NOT_NULL(rejectError);
}

void test_sensor_reading_filter_is_pass_through_with_default_config() {
    SensorReadingFilter filter;
    filter.configure(SensorFilterConfigV1{});

    TEST_ASSERT_EQUAL_FLOAT(10.0F, filter.apply(10.0F));
    TEST_ASSERT_EQUAL_FLOAT(20.0F, filter.apply(20.0F));
    TEST_ASSERT_EQUAL_FLOAT(-5.0F, filter.apply(-5.0F));
}

void test_sensor_reading_filter_applies_calibration_before_smoothing() {
    SensorFilterConfigV1 config{};
    config.calibrationFactor = 2.0F;
    config.calibrationOffset = 1.0F;
    SensorReadingFilter filter;
    filter.configure(config);

    // First call seeds the smoothed value directly with the calibrated reading.
    TEST_ASSERT_EQUAL_FLOAT(11.0F, filter.apply(5.0F));
}

void test_sensor_reading_filter_smooths_step_input_toward_target() {
    SensorFilterConfigV1 config{};
    config.smoothingWeight = 0.5F;
    SensorReadingFilter filter;
    filter.configure(config);

    filter.apply(0.0F);
    const float afterFirstStep = filter.apply(10.0F);
    TEST_ASSERT_EQUAL_FLOAT(5.0F, afterFirstStep);
    const float afterSecondStep = filter.apply(10.0F);
    TEST_ASSERT_EQUAL_FLOAT(7.5F, afterSecondStep);

    for (int i = 0; i < 20; ++i) {
        filter.apply(10.0F);
    }
    TEST_ASSERT_FLOAT_WITHIN(0.01F, 10.0F, filter.currentValue());
}

void test_sensor_reading_filter_reset_reseeds_instead_of_blending() {
    SensorReadingFilter filter;
    filter.configure(SensorFilterConfigV1{});

    filter.apply(100.0F);
    filter.reset();
    TEST_ASSERT_EQUAL_FLOAT(1.0F, filter.apply(1.0F));
}
