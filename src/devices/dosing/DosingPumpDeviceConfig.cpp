#include "devices/dosing/DosingPumpDeviceConfig.h"

#include "devices/core/ConfigCodec.h"

#include <cmath>
#include <cstdio>
#include <cstring>
#include <type_traits>

namespace ewfm {

namespace {
constexpr double kMaxSpeedMlPerSec = 65.535;
constexpr double kMaxDoseAmountMl = 655.35;

bool dosingScheduleModeFromByte(uint8_t value, DosingScheduleMode& mode) {
    switch (value) {
    case static_cast<uint8_t>(DosingScheduleMode::Daily):
        mode = DosingScheduleMode::Daily;
        return true;
    case static_cast<uint8_t>(DosingScheduleMode::Weekly):
        mode = DosingScheduleMode::Weekly;
        return true;
    default:
        return false;
    }
}

bool parseDoseTime(const JsonVariantConst& variant, uint16_t& minuteOfDay, const char*& error) {
    if (!variant.is<const char*>()) {
        error = "dosing pump dose time must be an HH:mm string";
        return false;
    }
    unsigned int hour = 0U;
    unsigned int minute = 0U;
    char trailing = '\0';
    const int parsed = std::sscanf(variant.as<const char*>(), "%2u:%2u%c", &hour, &minute, &trailing);
    if (parsed != 2 || hour > 23U || minute > 59U) {
        error = "dosing pump dose time must be HH:mm (00:00-23:59)";
        return false;
    }
    minuteOfDay = static_cast<uint16_t>(hour * 60U + minute);
    return true;
}

void writeDoseTime(uint16_t minuteOfDay, JsonObject output) {
    char text[8]{};
    std::snprintf(text, sizeof(text), "%02u:%02u", static_cast<unsigned int>(minuteOfDay / 60U),
                  static_cast<unsigned int>(minuteOfDay % 60U));
    output["time"] = JsonString(text, JsonString::Copied);
}

bool parseAmountMl(const JsonVariantConst& variant, uint16_t& amountCentiMl, const char*& error) {
    if (!variant.is<float>() && !variant.is<double>() && !variant.is<unsigned int>() && !variant.is<int>() && !variant.is<long>()) {
        error = "dosing pump dose amountMl must be numeric";
        return false;
    }
    const double amountMl = variant.as<double>();
    if (!(amountMl > 0.0) || amountMl > kMaxDoseAmountMl) {
        error = "dosing pump dose amountMl is out of range";
        return false;
    }
    const long rounded = std::lround(amountMl * 100.0);
    if (rounded < 1L || rounded > static_cast<long>(UINT16_MAX)) {
        error = "dosing pump dose amountMl is out of range";
        return false;
    }
    amountCentiMl = static_cast<uint16_t>(rounded);
    return true;
}

bool parseWeekDays(const JsonVariantConst& variant, uint8_t& weekDays, const char*& error) {
    if (variant.isNull()) {
        return true;
    }
    if (!variant.is<JsonArrayConst>()) {
        error = "dosing pump schedule daysOfWeek must be an array";
        return false;
    }
    uint8_t mask = 0U;
    for (JsonVariantConst dayVariant : variant.as<JsonArrayConst>()) {
        if (!dayVariant.is<int>()) {
            error = "dosing pump schedule daysOfWeek entries must be integers";
            return false;
        }
        const int day = dayVariant.as<int>();
        if (day < 0 || day > 6) {
            error = "dosing pump schedule daysOfWeek entries must be 0-6 (0=Sunday)";
            return false;
        }
        mask = static_cast<uint8_t>(mask | (1U << static_cast<uint8_t>(day)));
    }
    weekDays = mask;
    return true;
}

void writeWeekDays(JsonObject output, uint8_t weekDays) {
    JsonArray array = output.createNestedArray("daysOfWeek");
    for (uint8_t day = 0U; day < 7U; ++day) {
        if ((weekDays & (1U << day)) != 0U) {
            array.add(day);
        }
    }
}
} // namespace

static_assert(std::is_trivially_copyable<DosingPumpDeviceConfigV1>::value, "DosingPumpDeviceConfigV1 must be POD");
static_assert(sizeof(DosingPumpDeviceConfigV1::kMagic) - 1U + sizeof(DosingPumpDeviceConfigV1) <= kMaxDeviceConfigBytes,
              "DosingPumpDeviceConfigV1 exceeds device config bound");

bool dosingScheduleModeFromString(const char* value, DosingScheduleMode& mode) {
    if (value == nullptr || std::strcmp(value, "daily") == 0) {
        mode = DosingScheduleMode::Daily;
        return true;
    }
    if (std::strcmp(value, "weekly") == 0) {
        mode = DosingScheduleMode::Weekly;
        return true;
    }
    return false;
}

const char* dosingScheduleModeName(DosingScheduleMode mode) {
    switch (mode) {
    case DosingScheduleMode::Daily:
        return "daily";
    case DosingScheduleMode::Weekly:
        return "weekly";
    }
    return "daily";
}

bool DosingPumpDeviceConfigV1::parseJson(const JsonObjectConst& input, const char*& error) {
    if (!DeviceBaseConfigV1::parseJson(input, error)) {
        return false;
    }

    const JsonVariantConst speedVariant = input["dosingSpeedMlPerSec"];
    if (!speedVariant.isNull()) {
        if (!speedVariant.is<float>() && !speedVariant.is<double>() && !speedVariant.is<unsigned int>() && !speedVariant.is<int>() &&
            !speedVariant.is<long>()) {
            error = "dosing pump dosingSpeedMlPerSec must be numeric";
            return false;
        }
        const double speedMlPerSec = speedVariant.as<double>();
        if (!(speedMlPerSec > 0.0) || speedMlPerSec > kMaxSpeedMlPerSec) {
            error = "dosing pump dosingSpeedMlPerSec is out of range";
            return false;
        }
        const long rounded = std::lround(speedMlPerSec * 1000.0);
        if (rounded < 1L || rounded > static_cast<long>(UINT16_MAX)) {
            error = "dosing pump dosingSpeedMlPerSec is out of range";
            return false;
        }
        speedMilliMlPerSec = static_cast<uint16_t>(rounded);
    }

    const JsonObjectConst containerInput = input["container"].as<JsonObjectConst>();
    if (!containerInput.isNull()) {
        const long capacity = containerInput["capacityMl"] | static_cast<long>(containerCapacityMl);
        if (capacity < 1L || capacity > static_cast<long>(UINT16_MAX)) {
            error = "dosing pump container capacityMl is out of range";
            return false;
        }
        containerCapacityMl = static_cast<uint16_t>(capacity);

        const long threshold = containerInput["thresholdPercent"] | static_cast<long>(thresholdPercent);
        if (threshold < 0L || threshold > 100L) {
            error = "dosing pump container thresholdPercent must be 0-100";
            return false;
        }
        thresholdPercent = static_cast<uint8_t>(threshold);

        blockAutoWhenEmpty = (containerInput["blockAutoWhenEmpty"] | (blockAutoWhenEmpty != 0U)) ? 1U : 0U;
    }

    const JsonObjectConst scheduleInput = input["schedule"].as<JsonObjectConst>();
    if (!scheduleInput.isNull()) {
        DosingScheduleMode mode{};
        (void)dosingScheduleModeFromByte(scheduleMode, mode);
        const JsonVariantConst modeVariant = scheduleInput["mode"];
        if (!modeVariant.isNull()) {
            if (!modeVariant.is<const char*>()) {
                error = "dosing pump schedule mode must be a string";
                return false;
            }
            if (!dosingScheduleModeFromString(modeVariant.as<const char*>(), mode)) {
                error = "dosing pump schedule mode is invalid";
                return false;
            }
        }
        scheduleMode = static_cast<uint8_t>(mode);

        const long everyDaysValue = scheduleInput["everyDays"] | static_cast<long>(everyDays);
        if (everyDaysValue < 1L || everyDaysValue > static_cast<long>(kDosingPumpMaxEveryDays)) {
            error = "dosing pump schedule everyDays is out of range";
            return false;
        }
        everyDays = static_cast<uint8_t>(everyDaysValue);

        if (!parseWeekDays(scheduleInput["daysOfWeek"], daysOfWeekMask, error)) {
            return false;
        }

        const long anchorDayValue = scheduleInput["anchorDay"] | static_cast<long>(anchorDay);
        if (anchorDayValue < 0L || anchorDayValue > static_cast<long>(UINT16_MAX)) {
            error = "dosing pump schedule anchorDay is out of range";
            return false;
        }
        anchorDay = static_cast<uint16_t>(anchorDayValue);

        const JsonVariantConst dosesVariant = scheduleInput["doses"];
        if (!dosesVariant.isNull()) {
            if (!dosesVariant.is<JsonArrayConst>()) {
                error = "dosing pump schedule doses must be an array";
                return false;
            }
            const JsonArrayConst dosesArray = dosesVariant.as<JsonArrayConst>();
            if (dosesArray.size() > kMaxDosingPumpDoses) {
                error = "dosing pump schedule has too many doses";
                return false;
            }
            std::array<DosingPumpDoseV1, kMaxDosingPumpDoses> parsedDoses{};
            uint8_t parsedCount = 0U;
            for (JsonVariantConst doseVariant : dosesArray) {
                if (!doseVariant.is<JsonObjectConst>()) {
                    error = "dosing pump dose entries must be objects";
                    return false;
                }
                const JsonObjectConst doseInput = doseVariant.as<JsonObjectConst>();
                DosingPumpDoseV1 dose{};
                if (!parseDoseTime(doseInput["time"], dose.minuteOfDay, error)) {
                    return false;
                }
                if (!parseAmountMl(doseInput["amountMl"], dose.amountCentiMl, error)) {
                    return false;
                }
                parsedDoses[parsedCount] = dose;
                ++parsedCount;
            }
            doses = parsedDoses;
            doseCount = parsedCount;
        }
    }
    return true;
}

DeviceValidationResult DosingPumpDeviceConfigV1::validate() const {
    const DeviceValidationResult baseValidation = DeviceBaseConfigV1::validate();
    if (!baseValidation.ok()) {
        return baseValidation;
    }
    DosingScheduleMode mode{};
    if (!dosingScheduleModeFromByte(scheduleMode, mode)) {
        return {DeviceError::InvalidConfig, "dosing pump schedule mode is invalid"};
    }
    if (speedMilliMlPerSec == 0U) {
        return {DeviceError::InvalidConfig, "dosing pump speed must be positive"};
    }
    if (containerCapacityMl == 0U) {
        return {DeviceError::InvalidConfig, "dosing pump container capacity must be positive"};
    }
    if (thresholdPercent > 100U) {
        return {DeviceError::InvalidConfig, "dosing pump container thresholdPercent must be 0-100"};
    }
    if (everyDays < 1U || everyDays > kDosingPumpMaxEveryDays) {
        return {DeviceError::InvalidConfig, "dosing pump schedule everyDays is out of range"};
    }
    if (mode == DosingScheduleMode::Weekly && daysOfWeekMask == 0U) {
        return {DeviceError::InvalidConfig, "dosing pump weekly schedule requires at least one day"};
    }
    if (doseCount > kMaxDosingPumpDoses) {
        return {DeviceError::BoundsExceeded, "dosing pump schedule has too many doses"};
    }
    for (uint8_t index = 0U; index < doseCount; ++index) {
        const DosingPumpDoseV1& dose = doses[index];
        if (dose.minuteOfDay >= kDosingPumpMinutesPerDay) {
            return {DeviceError::InvalidConfig, "dosing pump dose time is invalid"};
        }
        if (dose.amountCentiMl == 0U) {
            return {DeviceError::InvalidConfig, "dosing pump dose amount must be positive"};
        }
        // Strictly increasing times keep the per-day fired bitmask semantics unambiguous.
        if (index > 0U && dose.minuteOfDay <= doses[index - 1U].minuteOfDay) {
            return {DeviceError::InvalidConfig, "dosing pump doses must be sorted by unique time"};
        }
    }
    return {};
}

void DosingPumpDeviceConfigV1::writeJson(JsonObject output) const {
    DeviceBaseConfigV1::writeJson(output);
    output["dosingSpeedMlPerSec"] = static_cast<float>(speedMilliMlPerSec) / 1000.0F;

    JsonObject container = output.createNestedObject("container");
    container["capacityMl"] = containerCapacityMl;
    container["thresholdPercent"] = thresholdPercent;
    container["blockAutoWhenEmpty"] = blockAutoWhenEmpty != 0U;

    DosingScheduleMode mode{};
    (void)dosingScheduleModeFromByte(scheduleMode, mode);
    JsonObject schedule = output.createNestedObject("schedule");
    schedule["mode"] = dosingScheduleModeName(mode);
    schedule["everyDays"] = everyDays;
    writeWeekDays(schedule, daysOfWeekMask);
    schedule["anchorDay"] = anchorDay;
    JsonArray dosesArray = schedule.createNestedArray("doses");
    for (uint8_t index = 0U; index < doseCount; ++index) {
        JsonObject doseOutput = dosesArray.createNestedObject();
        writeDoseTime(doses[index].minuteOfDay, doseOutput);
        doseOutput["amountMl"] = static_cast<float>(doses[index].amountCentiMl) / 100.0F;
    }
}

} // namespace ewfm
