#pragma once

#include "devices/dosing/journal/SegmentedDoseJournal.h"
#include "platform/DevDataPartition.h"

#if defined(ARDUINO) && !defined(UNIT_TEST)
#include <LittleFS.h>
#endif

namespace ewfm {

// ~16 KiB per segment: 1365 records each, ~2700 across both segments per device - 90+ days of
// history at ~30 doses/day - so the 256 KiB devdata partition fits roughly 7 devices' journals.
constexpr size_t kDoseJournalSegmentBytes = 16384;
constexpr size_t kDoseJournalRecordsPerSegment = kDoseJournalSegmentBytes / sizeof(DoseJournalRecordV1);
// LittleFS needs spare blocks for wear-leveled writes; stop appending (drop the record) rather
// than run the filesystem down to zero.
constexpr size_t kDoseJournalMinFreeBytes = 8192;

class LittleFsDoseJournalStorage final : public IDoseJournalSegmentStorage {
public:
#if defined(ARDUINO) && !defined(UNIT_TEST)
    // Injected by App, which owns the single mount of the shared devdata partition.
    explicit LittleFsDoseJournalStorage(fs::LittleFSFS& fs) : fs_(fs) {}
#endif

    // Assumes App has already mounted the devdata partition; creates the journal directory if
    // missing. Safe to call before any append.
    bool begin();

    size_t recordCount(uint32_t deviceId, uint8_t segment) const override;
    size_t readRecords(uint32_t deviceId, uint8_t segment, size_t firstIndex, size_t maxCount, DoseJournalRecordV1* out) const override;
    bool appendRecord(uint32_t deviceId, uint8_t segment, const DoseJournalRecordV1& record) override;
    bool clearSegment(uint32_t deviceId, uint8_t segment) override;
    bool removeDevice(uint32_t deviceId) override;
    void forEachDeviceId(DeviceVisitor visitor, void* context) const override;

private:
    static constexpr size_t kMaxPathBytes = 32;
    static void buildDevicePath(char (&out)[kMaxPathBytes], uint32_t deviceId);
    static void buildSegmentPath(char (&out)[kMaxPathBytes], uint32_t deviceId, uint8_t segment);

#if defined(ARDUINO) && !defined(UNIT_TEST)
    fs::LittleFSFS& fs_;
#endif
};

} // namespace ewfm
