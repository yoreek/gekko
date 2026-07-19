#include "integrations/mqtt/HaDeviceSettings.h"

#include "devices/core/DeviceBaseConfig.h"

namespace ewfm {

HaDeviceSettingsRecord loadHaDeviceSettings(DeviceScopedDataStore& store, DeviceId deviceId) {
    HaDeviceSettingsRecord record{};
    if (!store.load(deviceId, kHaDeviceSettingsDataType, record).ok()) {
        record = HaDeviceSettingsRecord{};
        record.deviceId = deviceId;
    }
    return record;
}

DeviceValidationResult saveHaDeviceSettings(DeviceScopedDataStore& store, DeviceId deviceId, bool enabled,
                                            const std::string& nameOverride) {
    HaDeviceSettingsRecord record{};
    record.deviceId = deviceId;
    record.enabled = enabled ? 1U : 0U;
    if (!copyBoundedText(record.nameOverride, nameOverride)) {
        return {DeviceError::BoundsExceeded, "ha name override exceeds supported length"};
    }
    return store.save(deviceId, kHaDeviceSettingsDataType, record);
}

std::string effectiveHaDeviceName(const HaDeviceSettingsRecord& settings, const char* fallbackName) {
    if (settings.nameOverride[0] != '\0') {
        return std::string(settings.nameOverride);
    }
    return fallbackName != nullptr ? std::string(fallbackName) : std::string();
}

} // namespace ewfm
