#include "devices/sensors/filter/SensorFilterConfig.h"

#include <cmath>
#include <type_traits>

namespace ewfm {

static_assert(std::is_trivially_copyable<SensorFilterConfigV1>::value, "SensorFilterConfigV1 must be POD");
static_assert(sizeof(SensorFilterConfigV1) == 12, "SensorFilterConfigV1 layout changed");

namespace {

bool parseFloatField(const JsonVariantConst& variant, float& value) {
    if (variant.isNull()) {
        return true;
    }
    if (!variant.is<float>() && !variant.is<double>() && !variant.is<int>()) {
        return false;
    }
    value = variant.as<float>();
    return true;
}

} // namespace

DeviceValidationResult SensorFilterConfigV1::validate() const {
    if (!std::isfinite(smoothingWeight) || smoothingWeight <= 0.0F || smoothingWeight > 1.0F) {
        return {DeviceError::InvalidConfig, "sensor filter smoothing weight is invalid"};
    }
    if (!std::isfinite(calibrationFactor) || calibrationFactor == 0.0F) {
        return {DeviceError::InvalidConfig, "sensor filter calibration factor is invalid"};
    }
    if (!std::isfinite(calibrationOffset)) {
        return {DeviceError::InvalidConfig, "sensor filter calibration offset is invalid"};
    }
    return {};
}

bool SensorFilterConfigV1::parseJson(const JsonObjectConst& input, const char*& error) {
    float weight = smoothingWeight;
    if (!parseFloatField(input["smoothingWeight"], weight)) {
        error = "sensor filter smoothing weight must be numeric";
        return false;
    }
    float factor = calibrationFactor;
    if (!parseFloatField(input["calibrationFactor"], factor)) {
        error = "sensor filter calibration factor must be numeric";
        return false;
    }
    float offset = calibrationOffset;
    if (!parseFloatField(input["calibrationOffset"], offset)) {
        error = "sensor filter calibration offset must be numeric";
        return false;
    }

    smoothingWeight = weight;
    calibrationFactor = factor;
    calibrationOffset = offset;

    const DeviceValidationResult result = validate();
    if (!result.ok()) {
        error = result.message;
        return false;
    }
    return true;
}

void SensorFilterConfigV1::writeJson(JsonObject output) const {
    output["smoothingWeight"] = smoothingWeight;
    output["calibrationFactor"] = calibrationFactor;
    output["calibrationOffset"] = calibrationOffset;
}

} // namespace ewfm
