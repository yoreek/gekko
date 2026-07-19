#pragma once

#include "devices/core/DeviceBaseConfig.h"

#include <ArduinoJson.h>
#include <array>
#include <cstddef>
#include <cstdint>

namespace ewfm {

constexpr DeviceTypeId kDosingPumpDeviceTypeId = 19;
constexpr uint32_t kDosingPumpDeviceConfigVersion = 1;
constexpr size_t kMaxDosingPumpDoses = 16;
// A due dose fires within [doseMinute, doseMinute + grace); once the grace window passes unfired
// (device was busy, powered off, or clock arrived late) the dose is dropped for the day rather
// than dosed hours late - conservative for reef chemistry.
constexpr uint8_t kDosingPumpGraceMinutes = 5;
constexpr uint16_t kDosingPumpMinutesPerDay = 1440U;
constexpr uint8_t kDosingPumpMaxEveryDays = 30;

enum class DosingScheduleMode : uint8_t {
    Daily = 0,
    Weekly = 1,
};

#pragma pack(push, 1)
// Fixed-point units keep the blob compact and float-free: amounts in 0.01 ml (centi-ml, max
// 655.35 ml per dose), pump speed in 0.001 ml/s (micro-liters per second, max 65.535 ml/s).
struct DosingPumpDoseV1 {
    uint16_t minuteOfDay{0}; // 0..1439, JSON "time": "HH:mm"
    uint16_t amountCentiMl{0};
};

struct DosingPumpDeviceConfigV1 : DeviceBaseConfigV1 {
    static constexpr char kMagic[] = "DPUMP-1";
    uint16_t speedMilliMlPerSec{1000}; // JSON dosingSpeedMlPerSec (float ml/s)
    uint16_t containerCapacityMl{1000};
    uint8_t thresholdPercent{10};
    uint8_t blockAutoWhenEmpty{1};
    uint8_t scheduleMode{static_cast<uint8_t>(DosingScheduleMode::Daily)};
    uint8_t everyDays{1};         // daily mode: dose every N days
    uint8_t daysOfWeekMask{0x7F}; // weekly mode: bit0=Sunday .. bit6=Saturday
    uint8_t doseCount{0};
    // Local days-since-1970 anchor for the everyDays cycle (fits uint16 until year 2149). Sent by
    // the SPA when a daily schedule is saved, so the cycle phase survives reboots.
    uint16_t anchorDay{0};
    std::array<DosingPumpDoseV1, kMaxDosingPumpDoses> doses{};

    bool parseJson(const JsonObjectConst& input, const char*& error);
    DeviceValidationResult validate() const;
    void writeJson(JsonObject output) const;
};
#pragma pack(pop)

constexpr size_t dosingPumpDeviceConfigSize(const DosingPumpDeviceConfigV1&) {
    return sizeof(DosingPumpDeviceConfigV1::kMagic) - 1U + sizeof(DosingPumpDeviceConfigV1);
}

bool dosingScheduleModeFromString(const char* value, DosingScheduleMode& mode);
const char* dosingScheduleModeName(DosingScheduleMode mode);

} // namespace ewfm
