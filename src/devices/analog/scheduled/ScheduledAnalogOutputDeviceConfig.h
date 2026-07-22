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

// Legacy persisted layout: kept only so an old "ASCHED-1" blob can be decoded and migrated to
// V2. Only the data + validate() survive; all JSON handling lives on V2.
struct [[deprecated("legacy persisted analog-schedule config; decode/migration only")]] ScheduledAnalogOutputDeviceConfigV1
    : DeviceBaseConfigV1 {
    static constexpr char kMagic[] = "ASCHED-1";
    ScheduledAnalogOutputPointV1 points[kMaxScheduledAnalogOutputPoints]{};

    DeviceValidationResult validate() const;
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
    EWFM_LEGACY_CONFIG_USE_BEGIN
    void migrateFrom(const ScheduledAnalogOutputDeviceConfigV1& legacy);
    EWFM_LEGACY_CONFIG_USE_END
};
#pragma pack(pop)

EWFM_LEGACY_CONFIG_USE_BEGIN
constexpr size_t scheduledAnalogOutputDeviceConfigSize(const ScheduledAnalogOutputDeviceConfigV1&) {
    return sizeof(ScheduledAnalogOutputDeviceConfigV1::kMagic) - 1U + sizeof(ScheduledAnalogOutputDeviceConfigV1);
}
EWFM_LEGACY_CONFIG_USE_END

constexpr size_t scheduledAnalogOutputDeviceConfigSize(const ScheduledAnalogOutputDeviceConfigV2&) {
    return sizeof(ScheduledAnalogOutputDeviceConfigV2::kMagic) - 1U + sizeof(ScheduledAnalogOutputDeviceConfigV2);
}

// Reusable schedule-point helpers, shared by the config codec and the schedule-preset controller
// so point JSON parsing, validation, and serialization live in exactly one place.
bool parseAnalogSchedulePoints(const JsonArrayConst& array, ScheduledAnalogOutputPointV1 (&out)[kMaxScheduledAnalogOutputPoints],
                               const char*& error);
DeviceValidationResult validateAnalogSchedulePoints(const ScheduledAnalogOutputPointV1 (&points)[kMaxScheduledAnalogOutputPoints]);
void writeAnalogSchedulePoints(JsonArray array, const ScheduledAnalogOutputPointV1 (&points)[kMaxScheduledAnalogOutputPoints]);

bool decodeScheduledAnalogOutputDeviceConfig(const uint8_t* blob, size_t size, ScheduledAnalogOutputDeviceConfigV2& config);

} // namespace ewfm
