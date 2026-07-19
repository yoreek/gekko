#include "devices/dosing/journal/SegmentedDoseJournal.h"

namespace ewfm {

namespace {
constexpr size_t kReadChunkRecords = 32;

uint32_t lastRecordEpoch(const IDoseJournalSegmentStorage& storage, uint32_t deviceId, uint8_t segment) {
    const size_t count = storage.recordCount(deviceId, segment);
    if (count == 0U) {
        return 0U;
    }
    DoseJournalRecordV1 record{};
    if (storage.readRecords(deviceId, segment, count - 1U, 1U, &record) != 1U) {
        return 0U;
    }
    return record.epoch;
}

struct AllDevicesContext {
    const SegmentedDoseJournal* journal;
    uint32_t sinceEpoch;
    IDoseJournal::Visitor visitor;
    void* context;
    bool keepGoing;
};
} // namespace

SegmentedDoseJournal::SegmentedDoseJournal(IDoseJournalSegmentStorage& storage, size_t recordsPerSegment)
    : storage_(storage), recordsPerSegment_(recordsPerSegment) {}

uint8_t SegmentedDoseJournal::activeSegment(uint32_t deviceId) const {
    const size_t count0 = storage_.recordCount(deviceId, 0U);
    const size_t count1 = storage_.recordCount(deviceId, 1U);
    if (count1 == 0U) {
        return 0U;
    }
    if (count0 == 0U) {
        return 1U;
    }
    return lastRecordEpoch(storage_, deviceId, 1U) >= lastRecordEpoch(storage_, deviceId, 0U) ? 1U : 0U;
}

bool SegmentedDoseJournal::append(const DoseJournalRecordV1& record) {
    if (record.deviceId == 0U) {
        return false;
    }
    uint8_t segment = activeSegment(record.deviceId);
    if (storage_.recordCount(record.deviceId, segment) >= recordsPerSegment_) {
        const uint8_t other = segment == 0U ? 1U : 0U;
        if (!storage_.clearSegment(record.deviceId, other)) {
            return false;
        }
        segment = other;
    }
    return storage_.appendRecord(record.deviceId, segment, record);
}

bool SegmentedDoseJournal::removeDevice(uint32_t deviceId) {
    if (deviceId == 0U) {
        return false;
    }
    return storage_.removeDevice(deviceId);
}

void SegmentedDoseJournal::forEachNewestFirst(uint32_t deviceId, uint32_t sinceEpoch, Visitor visitor, void* context) const {
    if (visitor == nullptr) {
        return;
    }
    if (deviceId != 0U) {
        (void)visitDeviceNewestFirst(deviceId, sinceEpoch, visitor, context);
        return;
    }
    AllDevicesContext allContext{this, sinceEpoch, visitor, context, true};
    storage_.forEachDeviceId(
        [](uint32_t entryDeviceId, void* rawContext) {
            AllDevicesContext& all = *static_cast<AllDevicesContext*>(rawContext);
            all.keepGoing = all.journal->visitDeviceNewestFirst(entryDeviceId, all.sinceEpoch, all.visitor, all.context);
            return all.keepGoing;
        },
        &allContext);
}

bool SegmentedDoseJournal::visitDeviceNewestFirst(uint32_t deviceId, uint32_t sinceEpoch, Visitor visitor, void* context) const {
    const uint8_t active = activeSegment(deviceId);
    if (!visitSegmentNewestFirst(deviceId, active, sinceEpoch, visitor, context)) {
        return false;
    }
    return visitSegmentNewestFirst(deviceId, active == 0U ? 1U : 0U, sinceEpoch, visitor, context);
}

bool SegmentedDoseJournal::visitSegmentNewestFirst(uint32_t deviceId, uint8_t segment, uint32_t sinceEpoch, Visitor visitor,
                                                   void* context) const {
    size_t remaining = storage_.recordCount(deviceId, segment);
    DoseJournalRecordV1 chunk[kReadChunkRecords]{};
    while (remaining > 0U) {
        const size_t chunkCount = remaining >= kReadChunkRecords ? kReadChunkRecords : remaining;
        const size_t firstIndex = remaining - chunkCount;
        const size_t read = storage_.readRecords(deviceId, segment, firstIndex, chunkCount, chunk);
        if (read != chunkCount) {
            return true; // storage shrank underneath us - stop this segment, not the caller
        }
        for (size_t offset = chunkCount; offset > 0U; --offset) {
            const DoseJournalRecordV1& record = chunk[offset - 1U];
            if (record.epoch < sinceEpoch) {
                continue;
            }
            if (!visitor(record, context)) {
                return false;
            }
        }
        remaining = firstIndex;
    }
    return true;
}

} // namespace ewfm
