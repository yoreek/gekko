#pragma once

#include "devices/dosing/journal/DoseJournal.h"

#include <cstddef>
#include <cstdint>

namespace ewfm {

// Backing store for per-device pairs of append-only record segments. Implementations: LittleFS
// files on device (LittleFsDoseJournalStorage, one directory per device id), plain vectors in
// native tests.
class IDoseJournalSegmentStorage {
public:
    // Return false from the visitor to stop the enumeration early.
    using DeviceVisitor = bool (*)(uint32_t deviceId, void* context);

    IDoseJournalSegmentStorage() = default;
    IDoseJournalSegmentStorage(const IDoseJournalSegmentStorage&) = delete;
    IDoseJournalSegmentStorage& operator=(const IDoseJournalSegmentStorage&) = delete;
    virtual ~IDoseJournalSegmentStorage() = default;

    virtual size_t recordCount(uint32_t deviceId, uint8_t segment) const = 0;
    // Reads up to maxCount records starting at firstIndex; returns how many were read.
    virtual size_t readRecords(uint32_t deviceId, uint8_t segment, size_t firstIndex, size_t maxCount, DoseJournalRecordV1* out) const = 0;
    virtual bool appendRecord(uint32_t deviceId, uint8_t segment, const DoseJournalRecordV1& record) = 0;
    virtual bool clearSegment(uint32_t deviceId, uint8_t segment) = 0;
    // Deletes every trace of the device (both segments and any per-device container).
    virtual bool removeDevice(uint32_t deviceId) = 0;
    // Enumerates device ids that currently have journal data (order unspecified).
    virtual void forEachDeviceId(DeviceVisitor visitor, void* context) const = 0;
};

// Per-device two-segment ring journal: appends fill the device's active segment; when it is full
// the *other* segment is truncated and becomes active, so at least one full segment of that
// device's history always survives a rotation. Records within a segment are chronological, which
// makes newest-first iteration a simple backward walk (active segment first, then the previous
// one). The active segment is derived from storage on every use (the segment holding the newest
// record), so there is no in-RAM state to recover after a reboot.
class SegmentedDoseJournal final : public IDoseJournal {
public:
    SegmentedDoseJournal(IDoseJournalSegmentStorage& storage, size_t recordsPerSegment);

    bool append(const DoseJournalRecordV1& record) override;
    void forEachNewestFirst(uint32_t deviceId, uint32_t sinceEpoch, Visitor visitor, void* context) const override;
    bool removeDevice(uint32_t deviceId) override;

    uint8_t activeSegment(uint32_t deviceId) const;

private:
    bool visitDeviceNewestFirst(uint32_t deviceId, uint32_t sinceEpoch, Visitor visitor, void* context) const;
    bool visitSegmentNewestFirst(uint32_t deviceId, uint8_t segment, uint32_t sinceEpoch, Visitor visitor, void* context) const;

    IDoseJournalSegmentStorage& storage_;
    size_t recordsPerSegment_;
};

} // namespace ewfm
