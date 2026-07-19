#include "devices/dosing/journal/SegmentedDoseJournal.h"

#include <array>
#include <map>
#include <unity.h>
#include <vector>

using namespace ewfm;

namespace {

class MemorySegmentStorage final : public IDoseJournalSegmentStorage {
public:
    size_t recordCount(uint32_t deviceId, uint8_t segment) const override {
        const auto it = devices.find(deviceId);
        return it == devices.end() ? 0U : it->second[segment].size();
    }

    size_t readRecords(uint32_t deviceId, uint8_t segment, size_t firstIndex, size_t maxCount, DoseJournalRecordV1* out) const override {
        const auto it = devices.find(deviceId);
        if (it == devices.end()) {
            return 0U;
        }
        const std::vector<DoseJournalRecordV1>& records = it->second[segment];
        size_t read = 0U;
        while (read < maxCount && firstIndex + read < records.size()) {
            out[read] = records[firstIndex + read];
            ++read;
        }
        return read;
    }

    bool appendRecord(uint32_t deviceId, uint8_t segment, const DoseJournalRecordV1& record) override {
        if (!appendOk) {
            return false;
        }
        devices[deviceId][segment].push_back(record);
        return true;
    }

    bool clearSegment(uint32_t deviceId, uint8_t segment) override {
        if (!clearOk) {
            return false;
        }
        const auto it = devices.find(deviceId);
        if (it != devices.end()) {
            it->second[segment].clear();
        }
        ++clearCount;
        return true;
    }

    bool removeDevice(uint32_t deviceId) override {
        devices.erase(deviceId);
        return true;
    }

    void forEachDeviceId(DeviceVisitor visitor, void* context) const override {
        for (const auto& entry : devices) {
            if (!visitor(entry.first, context)) {
                return;
            }
        }
    }

    std::map<uint32_t, std::array<std::vector<DoseJournalRecordV1>, 2>> devices{};
    bool appendOk{true};
    bool clearOk{true};
    int clearCount{0};
};

DoseJournalRecordV1 makeRecord(uint32_t epoch, uint32_t deviceId = 1U, uint16_t amountCentiMl = 100U) {
    DoseJournalRecordV1 record{};
    record.epoch = epoch;
    record.deviceId = deviceId;
    record.type = static_cast<uint8_t>(DoseJournalEntryType::Schedule);
    record.amountCentiMl = amountCentiMl;
    return record;
}

struct CollectContext {
    std::vector<DoseJournalRecordV1> entries{};
    size_t limit{SIZE_MAX};
};

bool collectVisitor(const DoseJournalRecordV1& record, void* contextPtr) {
    CollectContext& context = *static_cast<CollectContext*>(contextPtr);
    context.entries.push_back(record);
    return context.entries.size() < context.limit;
}

} // namespace

void test_dose_journal_appends_and_iterates_newest_first() {
    MemorySegmentStorage storage;
    SegmentedDoseJournal journal(storage, 4U);

    for (uint32_t epoch = 100U; epoch < 103U; ++epoch) {
        TEST_ASSERT_TRUE(journal.append(makeRecord(epoch)));
    }

    CollectContext collected{};
    journal.forEachNewestFirst(1U, 0U, &collectVisitor, &collected);
    TEST_ASSERT_EQUAL(3U, collected.entries.size());
    TEST_ASSERT_EQUAL_UINT32(102U, collected.entries[0].epoch);
    TEST_ASSERT_EQUAL_UINT32(100U, collected.entries[2].epoch);
}

void test_dose_journal_rotation_clears_other_segment_and_keeps_history() {
    MemorySegmentStorage storage;
    SegmentedDoseJournal journal(storage, 4U);

    // Fill segment 0 (4 records), then two more -> rotation into segment 1.
    for (uint32_t epoch = 100U; epoch < 106U; ++epoch) {
        TEST_ASSERT_TRUE(journal.append(makeRecord(epoch)));
    }
    TEST_ASSERT_EQUAL_UINT8(1U, journal.activeSegment(1U));
    TEST_ASSERT_EQUAL(4U, storage.devices[1U][0].size());
    TEST_ASSERT_EQUAL(2U, storage.devices[1U][1].size());

    CollectContext collected{};
    journal.forEachNewestFirst(1U, 0U, &collectVisitor, &collected);
    TEST_ASSERT_EQUAL(6U, collected.entries.size());
    TEST_ASSERT_EQUAL_UINT32(105U, collected.entries[0].epoch);
    TEST_ASSERT_EQUAL_UINT32(100U, collected.entries[5].epoch);

    // Fill segment 1 and rotate back: segment 0's old records are gone, history keeps rolling.
    for (uint32_t epoch = 106U; epoch < 109U; ++epoch) {
        TEST_ASSERT_TRUE(journal.append(makeRecord(epoch)));
    }
    TEST_ASSERT_EQUAL_UINT8(0U, journal.activeSegment(1U));
    TEST_ASSERT_EQUAL(2, storage.clearCount);
    collected.entries.clear();
    journal.forEachNewestFirst(1U, 0U, &collectVisitor, &collected);
    TEST_ASSERT_EQUAL(5U, collected.entries.size()); // 4 in seg1 + 1 in seg0
    TEST_ASSERT_EQUAL_UINT32(108U, collected.entries[0].epoch);
    TEST_ASSERT_EQUAL_UINT32(104U, collected.entries[4].epoch);
}

void test_dose_journal_resumes_active_segment_from_newest_record() {
    MemorySegmentStorage storage;
    storage.devices[1U][0] = {makeRecord(100U), makeRecord(101U), makeRecord(102U), makeRecord(103U)};
    storage.devices[1U][1] = {makeRecord(104U)};

    // No begin()/in-RAM state: the active segment is re-derived from storage on every use, which
    // is exactly the reboot-recovery case.
    SegmentedDoseJournal journal(storage, 4U);
    TEST_ASSERT_EQUAL_UINT8(1U, journal.activeSegment(1U));

    TEST_ASSERT_TRUE(journal.append(makeRecord(105U)));
    TEST_ASSERT_EQUAL(2U, storage.devices[1U][1].size());
    TEST_ASSERT_EQUAL(4U, storage.devices[1U][0].size());
}

void test_dose_journal_keeps_devices_isolated_and_supports_all_devices_read() {
    MemorySegmentStorage storage;
    SegmentedDoseJournal journal(storage, 2U);
    TEST_ASSERT_TRUE(journal.append(makeRecord(100U, 1U)));
    TEST_ASSERT_TRUE(journal.append(makeRecord(101U, 2U)));
    TEST_ASSERT_TRUE(journal.append(makeRecord(102U, 1U)));
    TEST_ASSERT_TRUE(journal.append(makeRecord(103U, 2U)));
    // Device 1 rotates; device 2's segments must be untouched by it.
    TEST_ASSERT_TRUE(journal.append(makeRecord(104U, 1U)));
    TEST_ASSERT_EQUAL_UINT8(1U, journal.activeSegment(1U));
    TEST_ASSERT_EQUAL_UINT8(0U, journal.activeSegment(2U));
    TEST_ASSERT_EQUAL(2U, storage.devices[2U][0].size());

    CollectContext device1{};
    journal.forEachNewestFirst(1U, 0U, &collectVisitor, &device1);
    TEST_ASSERT_EQUAL(3U, device1.entries.size());
    TEST_ASSERT_EQUAL_UINT32(104U, device1.entries[0].epoch);

    // deviceId 0 = all devices, sequentially (newest-first within each device).
    CollectContext all{};
    journal.forEachNewestFirst(0U, 0U, &collectVisitor, &all);
    TEST_ASSERT_EQUAL(5U, all.entries.size());

    CollectContext since{};
    journal.forEachNewestFirst(2U, 103U, &collectVisitor, &since);
    TEST_ASSERT_EQUAL(1U, since.entries.size());
    TEST_ASSERT_EQUAL_UINT32(103U, since.entries[0].epoch);

    CollectContext limited{};
    limited.limit = 1U;
    journal.forEachNewestFirst(1U, 0U, &collectVisitor, &limited);
    TEST_ASSERT_EQUAL(1U, limited.entries.size());
    TEST_ASSERT_EQUAL_UINT32(104U, limited.entries[0].epoch);
}

void test_dose_journal_remove_device_erases_only_that_device() {
    MemorySegmentStorage storage;
    SegmentedDoseJournal journal(storage, 4U);
    TEST_ASSERT_TRUE(journal.append(makeRecord(100U, 1U)));
    TEST_ASSERT_TRUE(journal.append(makeRecord(101U, 2U)));

    TEST_ASSERT_TRUE(journal.removeDevice(1U));
    TEST_ASSERT_FALSE(journal.removeDevice(0U));

    CollectContext all{};
    journal.forEachNewestFirst(0U, 0U, &collectVisitor, &all);
    TEST_ASSERT_EQUAL(1U, all.entries.size());
    TEST_ASSERT_EQUAL_UINT32(2U, all.entries[0].deviceId);
}

void test_dose_journal_failed_clear_drops_append_without_losing_history() {
    MemorySegmentStorage storage;
    SegmentedDoseJournal journal(storage, 2U);
    TEST_ASSERT_TRUE(journal.append(makeRecord(100U)));
    TEST_ASSERT_TRUE(journal.append(makeRecord(101U)));

    storage.clearOk = false;
    TEST_ASSERT_FALSE(journal.append(makeRecord(102U)));
    TEST_ASSERT_EQUAL_UINT8(0U, journal.activeSegment(1U));
    TEST_ASSERT_EQUAL(2U, storage.devices[1U][0].size());

    storage.clearOk = true;
    TEST_ASSERT_TRUE(journal.append(makeRecord(103U)));
    TEST_ASSERT_EQUAL_UINT8(1U, journal.activeSegment(1U));
}
