#pragma once

#include "devices/analog/AnalogOutputDeviceConfig.h"
#include "devices/core/DeviceBaseConfig.h"

#include <cstddef>
#include <cstdint>

namespace ewfm {

constexpr DeviceTypeId kScheduledAnalogOutputDeviceTypeId = 22;
constexpr uint32_t kScheduledAnalogOutputDeviceConfigVersion = 2;
constexpr uint8_t kMaxScheduledAnalogOutputPoints = 10U;
constexpr uint16_t kAnalogScheduleMinutesPerDay = 1440U;

#pragma pack(push, 1)
struct ScheduledAnalogOutputPointV1 {
    uint8_t deleted{1U};
    uint16_t minuteOfDay{0U};
    uint16_t state{0U};
};

struct ScheduledAnalogOutputDeviceConfigV1 : DeviceBaseConfigV1 {
    static constexpr char kMagic[] = "ASCHED-1";
    ScheduledAnalogOutputPointV1 points[kMaxScheduledAnalogOutputPoints]{};

    bool parseJson(const JsonObjectConst& input, const char*& error);
    DeviceValidationResult validate() const;
    void writeJson(JsonObject output) const;
};

struct ScheduledAnalogOutputDeviceConfigV2 : DeviceBaseConfigV1 {
    static constexpr char kMagic[] = "ASCHED-2";
    ScheduledAnalogOutputPointV1 points[kMaxScheduledAnalogOutputPoints]{{0U, 0U, kAnalogOutputLevelMax},
                                                                         {1U, 0U, 0U},
                                                                         {1U, 0U, 0U},
                                                                         {1U, 0U, 0U},
                                                                         {1U, 0U, 0U},
                                                                         {1U, 0U, 0U},
                                                                         {1U, 0U, 0U},
                                                                         {1U, 0U, 0U},
                                                                         {1U, 0U, 0U},
                                                                         {1U, 0U, 0U}};

    bool parseJson(const JsonObjectConst& input, const char*& error);
    DeviceValidationResult validate() const;
    void writeJson(JsonObject output) const;
    void migrateFrom(const ScheduledAnalogOutputDeviceConfigV1& legacy);
};
#pragma pack(pop)

constexpr size_t scheduledAnalogOutputDeviceConfigSize(const ScheduledAnalogOutputDeviceConfigV1&) {
    return sizeof(ScheduledAnalogOutputDeviceConfigV1::kMagic) - 1U + sizeof(ScheduledAnalogOutputDeviceConfigV1);
}

constexpr size_t scheduledAnalogOutputDeviceConfigSize(const ScheduledAnalogOutputDeviceConfigV2&) {
    return sizeof(ScheduledAnalogOutputDeviceConfigV2::kMagic) - 1U + sizeof(ScheduledAnalogOutputDeviceConfigV2);
}

bool decodeScheduledAnalogOutputDeviceConfig(const uint8_t* blob, size_t size, ScheduledAnalogOutputDeviceConfigV2& config);

} // namespace ewfm
