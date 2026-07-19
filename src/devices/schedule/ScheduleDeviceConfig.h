#pragma once

#include "devices/core/DeviceBaseConfig.h"

#include <ArduinoJson.h>
#include <array>
#include <cstddef>
#include <cstdint>

namespace ewfm {

constexpr DeviceTypeId kScheduleDeviceTypeId = 15;
constexpr uint32_t kScheduleDeviceConfigVersion = 1;
constexpr size_t kMaxScheduleRules = 4;
constexpr uint16_t kScheduleMinutesPerDay = 1440U;

enum class ScheduleRuleMode : uint8_t {
    AlwaysOn = 0,
    Interval = 1,
};

// Minute precision throughout, matching the reference design (Time::minutesOfDay()) - there is no
// seconds-of-day concept anywhere in a schedule rule.
#pragma pack(push, 1)
struct ScheduleRuleV1 {
    uint8_t enabled{1};
    uint8_t weekDays{0x7F}; // bit0=Sunday .. bit6=Saturday
    uint16_t startMinuteOfDay{0};
    uint16_t endMinuteOfDay{kScheduleMinutesPerDay - 1U};
    uint8_t mode{static_cast<uint8_t>(ScheduleRuleMode::AlwaysOn)};
    uint16_t intervalsPerWindow{1};
    uint16_t durationMinutes{1};
};

struct ScheduleDeviceConfigV1 : DeviceBaseConfigV1 {
    static constexpr char kMagic[] = "SCHED-1";
    uint8_t ruleCount{0};
    std::array<ScheduleRuleV1, kMaxScheduleRules> rules{};

    bool parseJson(const JsonObjectConst& input, const char*& error);
    DeviceValidationResult validate() const;
    void writeJson(JsonObject output) const;
};
#pragma pack(pop)

constexpr size_t scheduleDeviceConfigSize(const ScheduleDeviceConfigV1&) {
    return sizeof(ScheduleDeviceConfigV1::kMagic) - 1U + sizeof(ScheduleDeviceConfigV1);
}

bool scheduleRuleModeFromString(const char* value, ScheduleRuleMode& mode);
const char* scheduleRuleModeName(ScheduleRuleMode mode);

bool parseScheduleDeviceConfigJson(const JsonObjectConst& input, ScheduleDeviceConfigV1& config, const char*& error);
void writeScheduleDeviceConfigJson(const ScheduleDeviceConfigV1& config, JsonObject output);

} // namespace ewfm
