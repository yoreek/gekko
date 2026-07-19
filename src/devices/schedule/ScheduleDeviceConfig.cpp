#include "devices/schedule/ScheduleDeviceConfig.h"

#include "devices/core/ConfigCodec.h"

#include <cmath>
#include <cstring>
#include <type_traits>

namespace ewfm {

namespace {
bool scheduleRuleModeFromByte(uint8_t value, ScheduleRuleMode& mode) {
    switch (value) {
    case static_cast<uint8_t>(ScheduleRuleMode::AlwaysOn):
        mode = ScheduleRuleMode::AlwaysOn;
        return true;
    case static_cast<uint8_t>(ScheduleRuleMode::Interval):
        mode = ScheduleRuleMode::Interval;
        return true;
    default:
        return false;
    }
}

uint16_t scheduleRuleWindowMinutes(const ScheduleRuleV1& rule) {
    if (rule.startMinuteOfDay == rule.endMinuteOfDay) {
        return kScheduleMinutesPerDay;
    }
    if (rule.startMinuteOfDay < rule.endMinuteOfDay) {
        return rule.endMinuteOfDay - rule.startMinuteOfDay;
    }
    return static_cast<uint16_t>((kScheduleMinutesPerDay - rule.startMinuteOfDay) + rule.endMinuteOfDay);
}

bool parseWeekDays(const JsonVariantConst& variant, uint8_t& weekDays, const char*& error) {
    if (variant.isNull()) {
        return true;
    }
    if (!variant.is<JsonArrayConst>()) {
        error = "schedule rule weekDays must be an array";
        return false;
    }
    uint8_t mask = 0U;
    for (JsonVariantConst dayVariant : variant.as<JsonArrayConst>()) {
        if (!dayVariant.is<int>()) {
            error = "schedule rule weekDays entries must be integers";
            return false;
        }
        const int day = dayVariant.as<int>();
        if (day < 0 || day > 6) {
            error = "schedule rule weekDays entries must be 0-6 (0=Sunday)";
            return false;
        }
        mask = static_cast<uint8_t>(mask | (1U << static_cast<uint8_t>(day)));
    }
    weekDays = mask;
    return true;
}

void writeWeekDays(JsonObject output, uint8_t weekDays) {
    JsonArray array = output.createNestedArray("weekDays");
    for (uint8_t day = 0U; day < 7U; ++day) {
        if ((weekDays & (1U << day)) != 0U) {
            array.add(day);
        }
    }
}

// Schedules are minute-precision only (see ScheduleRuleV1). Any sub-minute value that arrives over
// the wire (e.g. a fractional-minute number) is rounded to the nearest whole minute here, at the
// one place values get saved into the persisted rule - callers never need to round themselves.
bool parseMinuteOfDay(const JsonVariantConst& variant, uint16_t& value, const char*& error) {
    if (variant.isNull()) {
        return true;
    }
    if (!variant.is<float>() && !variant.is<double>() && !variant.is<unsigned int>() && !variant.is<int>() && !variant.is<long>()) {
        error = "schedule rule time-of-day value must be numeric";
        return false;
    }
    const long rounded = std::lround(variant.as<double>());
    if (rounded < 0 || rounded >= static_cast<long>(kScheduleMinutesPerDay)) {
        error = "schedule rule time-of-day value must be 0-1439 minutes";
        return false;
    }
    value = static_cast<uint16_t>(rounded);
    return true;
}

bool parseIntervalsPerWindow(const JsonVariantConst& variant, uint16_t& value, const char*& error) {
    if (variant.isNull()) {
        return true;
    }
    if (!variant.is<unsigned int>() && !variant.is<int>() && !variant.is<long>()) {
        error = "schedule rule intervalsPerWindow must be numeric";
        return false;
    }
    const long parsed = variant.as<long>();
    if (parsed < 1 || parsed > static_cast<long>(UINT16_MAX)) {
        error = "schedule rule intervalsPerWindow is invalid";
        return false;
    }
    value = static_cast<uint16_t>(parsed);
    return true;
}

bool parseDurationMinutes(const JsonVariantConst& variant, uint16_t& value, const char*& error) {
    if (variant.isNull()) {
        return true;
    }
    if (!variant.is<float>() && !variant.is<double>() && !variant.is<unsigned int>() && !variant.is<int>() && !variant.is<long>()) {
        error = "schedule rule durationMinutes must be numeric";
        return false;
    }
    const long rounded = std::lround(variant.as<double>());
    if (rounded < 0 || rounded > static_cast<long>(UINT16_MAX)) {
        error = "schedule rule durationMinutes must be non-negative";
        return false;
    }
    value = static_cast<uint16_t>(rounded);
    return true;
}

bool parseScheduleRule(const JsonObjectConst& input, ScheduleRuleV1& rule, const char*& error) {
    rule.enabled = (input["enabled"] | (rule.enabled != 0U)) ? 1U : 0U;

    if (!parseWeekDays(input["weekDays"], rule.weekDays, error)) {
        return false;
    }
    if (!parseMinuteOfDay(input["startMinuteOfDay"], rule.startMinuteOfDay, error)) {
        return false;
    }
    if (!parseMinuteOfDay(input["endMinuteOfDay"], rule.endMinuteOfDay, error)) {
        return false;
    }

    ScheduleRuleMode mode{};
    (void)scheduleRuleModeFromByte(rule.mode, mode);
    const JsonVariantConst modeVariant = input["mode"];
    if (!modeVariant.isNull()) {
        if (!modeVariant.is<const char*>()) {
            error = "schedule rule mode must be a string";
            return false;
        }
        if (!scheduleRuleModeFromString(modeVariant.as<const char*>(), mode)) {
            error = "schedule rule mode is invalid";
            return false;
        }
    }
    rule.mode = static_cast<uint8_t>(mode);

    if (!parseIntervalsPerWindow(input["intervalsPerWindow"], rule.intervalsPerWindow, error)) {
        return false;
    }
    if (!parseDurationMinutes(input["durationMinutes"], rule.durationMinutes, error)) {
        return false;
    }
    return true;
}

void writeScheduleRule(const ScheduleRuleV1& rule, JsonObject output) {
    ScheduleRuleMode mode{};
    (void)scheduleRuleModeFromByte(rule.mode, mode);

    output["enabled"] = rule.enabled != 0U;
    writeWeekDays(output, rule.weekDays);
    output["startMinuteOfDay"] = rule.startMinuteOfDay;
    output["endMinuteOfDay"] = rule.endMinuteOfDay;
    output["mode"] = scheduleRuleModeName(mode);
    output["intervalsPerWindow"] = rule.intervalsPerWindow;
    output["durationMinutes"] = rule.durationMinutes;
}

DeviceValidationResult validateScheduleRule(const ScheduleRuleV1& rule) {
    ScheduleRuleMode mode{};
    if (!scheduleRuleModeFromByte(rule.mode, mode)) {
        return {DeviceError::InvalidConfig, "schedule rule mode is invalid"};
    }
    if (rule.startMinuteOfDay >= kScheduleMinutesPerDay || rule.endMinuteOfDay >= kScheduleMinutesPerDay) {
        return {DeviceError::InvalidConfig, "schedule rule time-of-day is invalid"};
    }
    if (mode == ScheduleRuleMode::Interval) {
        if (rule.intervalsPerWindow == 0U) {
            return {DeviceError::InvalidConfig, "schedule rule intervalsPerWindow must be at least 1"};
        }
        const uint16_t windowMinutes = scheduleRuleWindowMinutes(rule);
        const uint16_t intervalMinutes = static_cast<uint16_t>(windowMinutes / rule.intervalsPerWindow);
        if (intervalMinutes == 0U) {
            return {DeviceError::InvalidConfig, "schedule rule window is too short for intervalsPerWindow"};
        }
        if (rule.durationMinutes == 0U || rule.durationMinutes > intervalMinutes) {
            return {DeviceError::InvalidConfig, "schedule rule durationMinutes must fit within one interval slice"};
        }
    }
    return {};
}
} // namespace

static_assert(std::is_trivially_copyable<ScheduleDeviceConfigV1>::value, "ScheduleDeviceConfigV1 must be POD");
static_assert(sizeof(ScheduleDeviceConfigV1::kMagic) - 1U + sizeof(ScheduleDeviceConfigV1) <= kMaxDeviceConfigBytes,
              "ScheduleDeviceConfigV1 exceeds device config bound");

bool scheduleRuleModeFromString(const char* value, ScheduleRuleMode& mode) {
    if (value == nullptr || std::strcmp(value, "alwaysOn") == 0) {
        mode = ScheduleRuleMode::AlwaysOn;
        return true;
    }
    if (std::strcmp(value, "interval") == 0) {
        mode = ScheduleRuleMode::Interval;
        return true;
    }
    return false;
}

const char* scheduleRuleModeName(ScheduleRuleMode mode) {
    switch (mode) {
    case ScheduleRuleMode::AlwaysOn:
        return "alwaysOn";
    case ScheduleRuleMode::Interval:
        return "interval";
    }
    return "alwaysOn";
}

bool parseScheduleDeviceConfigJson(const JsonObjectConst& input, ScheduleDeviceConfigV1& config, const char*& error) {
    return config.parseJson(input, error);
}

void writeScheduleDeviceConfigJson(const ScheduleDeviceConfigV1& config, JsonObject output) {
    config.writeJson(output);
}

bool ScheduleDeviceConfigV1::parseJson(const JsonObjectConst& input, const char*& error) {
    if (!DeviceBaseConfigV1::parseJson(input, error)) {
        return false;
    }

    const JsonVariantConst rulesVariant = input["rules"];
    if (rulesVariant.isNull()) {
        return true;
    }
    if (!rulesVariant.is<JsonArrayConst>()) {
        error = "schedule rules must be an array";
        return false;
    }
    const JsonArrayConst rulesArray = rulesVariant.as<JsonArrayConst>();
    if (rulesArray.size() > kMaxScheduleRules) {
        error = "schedule has too many rules";
        return false;
    }

    std::array<ScheduleRuleV1, kMaxScheduleRules> parsedRules{};
    uint8_t parsedCount = 0U;
    for (JsonVariantConst ruleVariant : rulesArray) {
        if (!ruleVariant.is<JsonObjectConst>()) {
            error = "schedule rule entries must be objects";
            return false;
        }
        ScheduleRuleV1 rule{};
        if (!parseScheduleRule(ruleVariant.as<JsonObjectConst>(), rule, error)) {
            return false;
        }
        parsedRules[parsedCount] = rule;
        ++parsedCount;
    }

    rules = parsedRules;
    ruleCount = parsedCount;
    return true;
}

DeviceValidationResult ScheduleDeviceConfigV1::validate() const {
    const DeviceValidationResult baseValidation = DeviceBaseConfigV1::validate();
    if (!baseValidation.ok()) {
        return baseValidation;
    }
    if (ruleCount > kMaxScheduleRules) {
        return {DeviceError::BoundsExceeded, "schedule has too many rules"};
    }
    for (uint8_t index = 0U; index < ruleCount; ++index) {
        const DeviceValidationResult ruleValidation = validateScheduleRule(rules[index]);
        if (!ruleValidation.ok()) {
            return ruleValidation;
        }
    }
    return {};
}

void ScheduleDeviceConfigV1::writeJson(JsonObject output) const {
    DeviceBaseConfigV1::writeJson(output);
    JsonArray rulesArray = output.createNestedArray("rules");
    for (uint8_t index = 0U; index < ruleCount; ++index) {
        writeScheduleRule(rules[index], rulesArray.createNestedObject());
    }
}

} // namespace ewfm
