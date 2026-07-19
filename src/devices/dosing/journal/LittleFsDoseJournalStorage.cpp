#include "devices/dosing/journal/LittleFsDoseJournalStorage.h"

#include <cinttypes>
#include <cstdio>
#include <cstdlib>

namespace ewfm {

void LittleFsDoseJournalStorage::buildDevicePath(char (&out)[kMaxPathBytes], uint32_t deviceId) {
    (void)std::snprintf(out, sizeof(out), "/dj/%" PRIu32, deviceId);
}

void LittleFsDoseJournalStorage::buildSegmentPath(char (&out)[kMaxPathBytes], uint32_t deviceId, uint8_t segment) {
    (void)std::snprintf(out, sizeof(out), "/dj/%" PRIu32 "/seg%u.bin", deviceId, segment == 0U ? 0U : 1U);
}

#if defined(ARDUINO) && !defined(UNIT_TEST)

bool LittleFsDoseJournalStorage::begin() {
    // formatOnFail covers the first boot after the devdata partition is introduced (the region
    // holds leftover app bytes until formatted) - nothing to migrate, the journal starts empty.
    if (!fs_.begin(true, "/devdata", 4, kDeviceDataPartitionLabel)) {
        return false;
    }
    if (!fs_.exists("/dj")) {
        return fs_.mkdir("/dj");
    }
    return true;
}

size_t LittleFsDoseJournalStorage::recordCount(uint32_t deviceId, uint8_t segment) const {
    char path[kMaxPathBytes]{};
    buildSegmentPath(path, deviceId, segment);
    File file = fs_.open(path, "r");
    if (!file) {
        return 0U;
    }
    const size_t size = file.size();
    file.close();
    return size / sizeof(DoseJournalRecordV1);
}

size_t LittleFsDoseJournalStorage::readRecords(uint32_t deviceId, uint8_t segment, size_t firstIndex, size_t maxCount,
                                               DoseJournalRecordV1* out) const {
    if (out == nullptr || maxCount == 0U) {
        return 0U;
    }
    char path[kMaxPathBytes]{};
    buildSegmentPath(path, deviceId, segment);
    File file = fs_.open(path, "r");
    if (!file) {
        return 0U;
    }
    if (!file.seek(firstIndex * sizeof(DoseJournalRecordV1))) {
        file.close();
        return 0U;
    }
    const size_t bytesRead = file.read(reinterpret_cast<uint8_t*>(out), maxCount * sizeof(DoseJournalRecordV1));
    file.close();
    return bytesRead / sizeof(DoseJournalRecordV1);
}

bool LittleFsDoseJournalStorage::appendRecord(uint32_t deviceId, uint8_t segment, const DoseJournalRecordV1& record) {
    // Free-space guard: dropping a journal record is always preferable to wearing the filesystem
    // down to zero free blocks.
    const size_t totalBytes = fs_.totalBytes();
    const size_t usedBytes = fs_.usedBytes();
    if (totalBytes <= usedBytes || totalBytes - usedBytes < kDoseJournalMinFreeBytes) {
        return false;
    }
    char devicePath[kMaxPathBytes]{};
    buildDevicePath(devicePath, deviceId);
    if (!fs_.exists(devicePath) && !fs_.mkdir(devicePath)) {
        return false;
    }
    char path[kMaxPathBytes]{};
    buildSegmentPath(path, deviceId, segment);
    File file = fs_.open(path, "a");
    if (!file) {
        return false;
    }
    const size_t written = file.write(reinterpret_cast<const uint8_t*>(&record), sizeof(record));
    file.close();
    return written == sizeof(record);
}

bool LittleFsDoseJournalStorage::clearSegment(uint32_t deviceId, uint8_t segment) {
    char path[kMaxPathBytes]{};
    buildSegmentPath(path, deviceId, segment);
    if (!fs_.exists(path)) {
        return true;
    }
    return fs_.remove(path);
}

bool LittleFsDoseJournalStorage::removeDevice(uint32_t deviceId) {
    if (!clearSegment(deviceId, 0U) || !clearSegment(deviceId, 1U)) {
        return false;
    }
    char devicePath[kMaxPathBytes]{};
    buildDevicePath(devicePath, deviceId);
    if (!fs_.exists(devicePath)) {
        return true;
    }
    return fs_.rmdir(devicePath);
}

void LittleFsDoseJournalStorage::forEachDeviceId(DeviceVisitor visitor, void* context) const {
    if (visitor == nullptr) {
        return;
    }
    File root = fs_.open("/dj");
    if (!root || !root.isDirectory()) {
        return;
    }
    for (File entry = root.openNextFile(); entry; entry = root.openNextFile()) {
        if (!entry.isDirectory()) {
            continue;
        }
        const uint32_t deviceId = static_cast<uint32_t>(std::strtoul(entry.name(), nullptr, 10));
        if (deviceId == 0U) {
            continue;
        }
        if (!visitor(deviceId, context)) {
            break;
        }
    }
    root.close();
}

#else

bool LittleFsDoseJournalStorage::begin() {
    return true;
}

size_t LittleFsDoseJournalStorage::recordCount(uint32_t, uint8_t) const {
    return 0U;
}

size_t LittleFsDoseJournalStorage::readRecords(uint32_t, uint8_t, size_t, size_t, DoseJournalRecordV1*) const {
    return 0U;
}

bool LittleFsDoseJournalStorage::appendRecord(uint32_t, uint8_t, const DoseJournalRecordV1&) {
    return false;
}

bool LittleFsDoseJournalStorage::clearSegment(uint32_t, uint8_t) {
    return true;
}

bool LittleFsDoseJournalStorage::removeDevice(uint32_t) {
    return true;
}

void LittleFsDoseJournalStorage::forEachDeviceId(DeviceVisitor, void*) const {}

#endif

} // namespace ewfm
