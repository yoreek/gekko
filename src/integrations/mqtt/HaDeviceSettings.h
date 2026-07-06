#pragma once

#include "devices/core/DeviceTypes.h"
#include "devices/registry/DeviceScopedDataStore.h"

#include <cstdint>
#include <string>

namespace ewfm {

constexpr size_t kHaDeviceNameOverrideLength = 48;
constexpr uint16_t kHaDeviceSettingsRecordVersion = 1;
constexpr const char* kHaDeviceSettingsDataType = "ha_settings";

// Per-device Home Assistant opt-in, stored independently of the device's own type-specific
// config blob (see the "Per-device HA opt-in и имя entity" section of the implementation plan).
// Absent for a device until the user explicitly opts in via the "setHaSettings" command.
#pragma pack(push, 1)
struct HaDeviceSettingsRecord {
    uint16_t recordVersion{kHaDeviceSettingsRecordVersion};
    DeviceId deviceId{0};
    uint8_t enabled{0};
    char nameOverride[kHaDeviceNameOverrideLength + 1]{};
};
#pragma pack(pop)

// Returns a disabled/default record (not an error) when nothing has been saved yet for deviceId.
HaDeviceSettingsRecord loadHaDeviceSettings(DeviceScopedDataStore& store, DeviceId deviceId);
DeviceValidationResult saveHaDeviceSettings(DeviceScopedDataStore& store, DeviceId deviceId, bool enabled, const std::string& nameOverride);

// nameOverride if set, otherwise fallbackName (the device's own base config name).
std::string effectiveHaDeviceName(const HaDeviceSettingsRecord& settings, const char* fallbackName);

} // namespace ewfm
