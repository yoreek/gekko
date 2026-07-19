#pragma once

#include "devices/core/DeviceBaseConfig.h"

#include <ArduinoJson.h>
#include <cstddef>
#include <cstdint>

namespace ewfm {

constexpr DeviceTypeId kAutoSwitchDeviceTypeId = 16;
constexpr uint32_t kAutoSwitchDeviceConfigVersion = 1;
constexpr uint32_t kAutoSwitchDefaultPauseDurationSeconds = 3600;
constexpr uint32_t kAutoSwitchMinPauseDurationSeconds = 1;
constexpr uint32_t kAutoSwitchMaxPauseDurationSeconds = 30UL * 24UL * 3600UL; // 30 days
// Bounds how many Condition-role AND dependencies an AutoSwitchDevice can have, in addition to its
// one required target Switch dependency (7 of the 16 total DeviceRegistryEntry.deps slots).
constexpr size_t kMaxAutoSwitchConditions = 6;

// User-controlled intent, mirrors ReefDuino's ScheduledSwitchMode (TurnedOnMode/TurnedOffMode/
// ScheduledMode/PausedMode) as one flat enum rather than a mode plus a separate overlay flag:
// Off/On force the target switch regardless of the conditions; Auto follows the logical AND of all
// attached Condition-role dependencies (Off if none are attached); Paused is only reachable from
// Auto and forces the target off for a bounded duration, then reverts to Auto (see
// AutoSwitchDevice::handleCommand()). Off/On/
// Auto are persisted as retained state (like SwitchDeviceBase's outputState), not inside the
// versioned config blob; Paused is deliberately never persisted (its deadline is uptime-relative
// and can't survive a reboot meaningfully) -- see AutoSwitchDevice::saveRetainedState().
enum class AutoSwitchMode : uint8_t {
    Off = 0,
    On = 1,
    Auto = 2,
    Paused = 3,
};

#pragma pack(push, 1)
struct AutoSwitchDeviceConfigV1 : DeviceBaseConfigV1 {
    static constexpr char kMagic[] = "AUTOSW-1";
    uint32_t pauseDurationSeconds{kAutoSwitchDefaultPauseDurationSeconds};

    bool parseJson(const JsonObjectConst& input, const char*& error);
    DeviceValidationResult validate() const;
    void writeJson(JsonObject output) const;
};
#pragma pack(pop)

constexpr size_t autoSwitchDeviceConfigSize(const AutoSwitchDeviceConfigV1&) {
    return sizeof(AutoSwitchDeviceConfigV1::kMagic) - 1U + sizeof(AutoSwitchDeviceConfigV1);
}

bool autoSwitchModeFromString(const char* value, AutoSwitchMode& mode);
const char* autoSwitchModeName(AutoSwitchMode mode);

bool parseAutoSwitchDeviceConfigJson(const JsonObjectConst& input, AutoSwitchDeviceConfigV1& config, const char*& error);
void writeAutoSwitchDeviceConfigJson(const AutoSwitchDeviceConfigV1& config, JsonObject output);

} // namespace ewfm
