#include "devices/analog/scheduled/ScheduledAnalogOutputDeviceConfig.h"

#include "devices/analog/AnalogOutputDeviceConfig.h"
#include "devices/core/ConfigCodec.h"

#include <cstring>
#include <type_traits>

namespace ewfm {

EWFM_LEGACY_CONFIG_USE_BEGIN
static_assert(std::is_trivially_copyable<ScheduledAnalogOutputDeviceConfigV1>::value, "ScheduledAnalogOutputDeviceConfigV1 must be POD");
static_assert(sizeof(ScheduledAnalogOutputDeviceConfigV1) == 84U, "ScheduledAnalogOutputDeviceConfigV1 layout changed");
static_assert(sizeof(ScheduledAnalogOutputDeviceConfigV1::kMagic) - 1U + sizeof(ScheduledAnalogOutputDeviceConfigV1) <=
                  kMaxDeviceConfigBytes,
              "ScheduledAnalogOutputDeviceConfigV1 exceeds device config bound");
EWFM_LEGACY_CONFIG_USE_END
static_assert(std::is_trivially_copyable<ScheduledAnalogOutputDeviceConfigV2>::value, "ScheduledAnalogOutputDeviceConfigV2 must be POD");
static_assert(sizeof(ScheduledAnalogOutputDeviceConfigV2) == 84U, "ScheduledAnalogOutputDeviceConfigV2 layout changed");
static_assert(sizeof(ScheduledAnalogOutputDeviceConfigV2::kMagic) - 1U + sizeof(ScheduledAnalogOutputDeviceConfigV2) <=
                  kMaxDeviceConfigBytes,
              "ScheduledAnalogOutputDeviceConfigV2 exceeds device config bound");

namespace {
bool parsePoint(const JsonObjectConst& input, ScheduledAnalogOutputPointV1& point, const char*& error) {
    point.deleted = (input["deleted"] | (point.deleted != 0U)) ? 1U : 0U;
    const JsonVariantConst minute = input["minuteOfDay"];
    if (!minute.isNull()) {
        if (!minute.is<unsigned int>() && !minute.is<int>() && !minute.is<long>()) {
            error = "analog schedule point time must be numeric";
            return false;
        }
        const long parsed = minute.as<long>();
        if (parsed < 0L || parsed >= static_cast<long>(kAnalogScheduleMinutesPerDay)) {
            error = "analog schedule point time is out of bounds";
            return false;
        }
        point.minuteOfDay = static_cast<uint16_t>(parsed);
    }
    return input["state"].isNull() || OutputDeviceValueCodec<uint16_t>::parseJson(input["state"], point.state, error);
}
} // namespace

EWFM_LEGACY_CONFIG_USE_BEGIN
DeviceValidationResult ScheduledAnalogOutputDeviceConfigV1::validate() const {
    const DeviceValidationResult baseValidation = DeviceBaseConfigV1::validate();
    if (!baseValidation.ok()) {
        return baseValidation;
    }
    for (uint8_t index = 0U; index < kMaxScheduledAnalogOutputPoints; ++index) {
        const ScheduledAnalogOutputPointV1& point = points[index];
        if (point.deleted > 1U) {
            return {DeviceError::InvalidConfig, "analog schedule point is invalid"};
        }
        if (point.deleted != 0U) {
            continue;
        }
        if (point.minuteOfDay >= kAnalogScheduleMinutesPerDay || point.state > kAnalogOutputLevelMax) {
            return {DeviceError::InvalidConfig, "analog schedule point is invalid"};
        }
        for (uint8_t other = static_cast<uint8_t>(index + 1U); other < kMaxScheduledAnalogOutputPoints; ++other) {
            if (points[other].deleted == 0U && points[other].minuteOfDay == point.minuteOfDay) {
                return {DeviceError::InvalidConfig, "analog schedule contains duplicate active times"};
            }
        }
    }
    return {};
}
EWFM_LEGACY_CONFIG_USE_END

bool ScheduledAnalogOutputDeviceConfigV2::parseJson(const JsonObjectConst& input, const char*& error) {
    if (!DeviceBaseConfigV1::parseJson(input, error)) {
        return false;
    }
    const JsonVariantConst pointsInput = input["points"];
    if (pointsInput.isNull()) {
        return true;
    }
    if (!pointsInput.is<JsonArrayConst>()) {
        error = "analog schedule points must be an array";
        return false;
    }
    const JsonArrayConst array = pointsInput.as<JsonArrayConst>();
    if (array.size() == 0U) {
        error = "analog schedule requires at least one point";
        return false;
    }
    if (array.size() > kMaxScheduledAnalogOutputPoints) {
        error = "analog schedule has too many points";
        return false;
    }
    ScheduledAnalogOutputPointV1 parsed[kMaxScheduledAnalogOutputPoints]{};
    for (ScheduledAnalogOutputPointV1& point : parsed) {
        point.deleted = 1U;
    }
    uint8_t count = 0U;
    for (JsonVariantConst item : array) {
        if (!item.is<JsonObjectConst>()) {
            error = "analog schedule point must be an object";
            return false;
        }
        parsed[count].deleted = 0U;
        if (!parsePoint(item.as<JsonObjectConst>(), parsed[count], error)) {
            return false;
        }
        ++count;
    }
    std::memcpy(points, parsed, sizeof(points));
    return true;
}

DeviceValidationResult ScheduledAnalogOutputDeviceConfigV2::validate() const {
    const DeviceValidationResult baseValidation = DeviceBaseConfigV1::validate();
    if (!baseValidation.ok()) {
        return baseValidation;
    }
    uint8_t activeCount = 0U;
    for (uint8_t index = 0U; index < kMaxScheduledAnalogOutputPoints; ++index) {
        const ScheduledAnalogOutputPointV1& point = points[index];
        if (point.deleted > 1U) {
            return {DeviceError::InvalidConfig, "analog schedule point is invalid"};
        }
        if (point.deleted != 0U) {
            continue;
        }
        ++activeCount;
        if (point.minuteOfDay >= kAnalogScheduleMinutesPerDay || point.state > kAnalogOutputLevelMax) {
            return {DeviceError::InvalidConfig, "analog schedule point is invalid"};
        }
        for (uint8_t other = static_cast<uint8_t>(index + 1U); other < kMaxScheduledAnalogOutputPoints; ++other) {
            if (points[other].deleted == 0U && points[other].minuteOfDay == point.minuteOfDay) {
                return {DeviceError::InvalidConfig, "analog schedule contains duplicate active times"};
            }
        }
    }
    return activeCount == 0U ? DeviceValidationResult{DeviceError::InvalidConfig, "analog schedule requires at least one point"}
                             : DeviceValidationResult{};
}

void ScheduledAnalogOutputDeviceConfigV2::writeJson(JsonObject output) const {
    DeviceBaseConfigV1::writeJson(output);
    JsonArray array = output.createNestedArray("points");
    for (uint8_t index = 0U; index < kMaxScheduledAnalogOutputPoints; ++index) {
        if (points[index].deleted != 0U) {
            continue;
        }
        JsonObject point = array.createNestedObject();
        point["deleted"] = false;
        point["minuteOfDay"] = points[index].minuteOfDay;
        OutputDeviceValueCodec<uint16_t>::writeJson(point, "state", points[index].state);
    }
}

EWFM_LEGACY_CONFIG_USE_BEGIN
void ScheduledAnalogOutputDeviceConfigV2::migrateFrom(const ScheduledAnalogOutputDeviceConfigV1& legacy) {
    enabled = legacy.enabled;
    std::memcpy(name, legacy.name, sizeof(name));
    std::memcpy(points, legacy.points, sizeof(points));
    for (const ScheduledAnalogOutputPointV1& point : points) {
        if (point.deleted == 0U) {
            return;
        }
    }
    points[0] = {0U, 0U, kAnalogOutputLevelMax};
}
EWFM_LEGACY_CONFIG_USE_END

bool decodeScheduledAnalogOutputDeviceConfig(const uint8_t* blob, const size_t size, ScheduledAnalogOutputDeviceConfigV2& config) {
    if (decodeFixedConfigBlob(ScheduledAnalogOutputDeviceConfigV2::kMagic, blob, size, config) && config.validate().ok()) {
        return true;
    }
    EWFM_LEGACY_CONFIG_USE_BEGIN
    ScheduledAnalogOutputDeviceConfigV1 legacy{};
    if (!decodeFixedConfigBlob(ScheduledAnalogOutputDeviceConfigV1::kMagic, blob, size, legacy) || !legacy.validate().ok()) {
        return false;
    }
    EWFM_LEGACY_CONFIG_USE_END
    config.migrateFrom(legacy);
    return config.validate().ok();
}

} // namespace ewfm
