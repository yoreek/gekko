#include "devices/analog/scheduled/presets/LittleFsSchedulePresetStorage.h"

#include <cinttypes>
#include <cstdio>

namespace ewfm {

void LittleFsSchedulePresetStorage::buildDevicePath(char (&out)[kMaxPathBytes], uint32_t deviceId) {
    (void)std::snprintf(out, sizeof(out), "/sap/%" PRIu32, deviceId);
}

void LittleFsSchedulePresetStorage::buildSlotPath(char (&out)[kMaxPathBytes], uint32_t deviceId, uint8_t slot) {
    (void)std::snprintf(out, sizeof(out), "/sap/%" PRIu32 "/p%u.bin", deviceId, static_cast<unsigned>(slot));
}

#if defined(ARDUINO) && !defined(UNIT_TEST)

bool LittleFsSchedulePresetStorage::begin() {
    // Assumes App has already mounted the shared devdata partition; only responsible for this
    // feature's own top-level directory.
    if (!fs_.exists("/sap")) {
        return fs_.mkdir("/sap");
    }
    return true;
}

bool LittleFsSchedulePresetStorage::save(uint32_t deviceId, uint8_t slot, const SchedulePresetRecordV1& record) {
    if (slot >= kMaxSchedulePresets) {
        return false;
    }
    const size_t totalBytes = fs_.totalBytes();
    const size_t usedBytes = fs_.usedBytes();
    if (totalBytes <= usedBytes || totalBytes - usedBytes < kSchedulePresetMinFreeBytes) {
        return false;
    }
    char devicePath[kMaxPathBytes]{};
    buildDevicePath(devicePath, deviceId);
    if (!fs_.exists(devicePath) && !fs_.mkdir(devicePath)) {
        return false;
    }
    char path[kMaxPathBytes]{};
    buildSlotPath(path, deviceId, slot);
    File file = fs_.open(path, "w");
    if (!file) {
        return false;
    }
    const size_t written = file.write(reinterpret_cast<const uint8_t*>(&record), sizeof(record));
    file.close();
    return written == sizeof(record);
}

bool LittleFsSchedulePresetStorage::load(uint32_t deviceId, uint8_t slot, SchedulePresetRecordV1& out) const {
    if (slot >= kMaxSchedulePresets) {
        return false;
    }
    char path[kMaxPathBytes]{};
    buildSlotPath(path, deviceId, slot);
    File file = fs_.open(path, "r");
    if (!file) {
        return false;
    }
    const size_t read = file.read(reinterpret_cast<uint8_t*>(&out), sizeof(out));
    file.close();
    return read == sizeof(out) && schedulePresetRecordValid(out);
}

bool LittleFsSchedulePresetStorage::erase(uint32_t deviceId, uint8_t slot) {
    if (slot >= kMaxSchedulePresets) {
        return false;
    }
    char path[kMaxPathBytes]{};
    buildSlotPath(path, deviceId, slot);
    if (!fs_.exists(path)) {
        return true;
    }
    return fs_.remove(path);
}

bool LittleFsSchedulePresetStorage::removeDevice(uint32_t deviceId) {
    for (uint8_t slot = 0U; slot < kMaxSchedulePresets; ++slot) {
        if (!erase(deviceId, slot)) {
            return false;
        }
    }
    char devicePath[kMaxPathBytes]{};
    buildDevicePath(devicePath, deviceId);
    if (!fs_.exists(devicePath)) {
        return true;
    }
    return fs_.rmdir(devicePath);
}

#else

bool LittleFsSchedulePresetStorage::begin() {
    return true;
}

bool LittleFsSchedulePresetStorage::save(uint32_t, uint8_t, const SchedulePresetRecordV1&) {
    return false;
}

bool LittleFsSchedulePresetStorage::load(uint32_t, uint8_t, SchedulePresetRecordV1&) const {
    return false;
}

bool LittleFsSchedulePresetStorage::erase(uint32_t, uint8_t) {
    return true;
}

bool LittleFsSchedulePresetStorage::removeDevice(uint32_t) {
    return true;
}

#endif

} // namespace ewfm
