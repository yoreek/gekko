#pragma once

#include <cstddef>
#include <cstdint>

namespace ewfm {

enum class DoseJournalEntryType : uint8_t {
    Schedule = 0,
    Manual = 1,
};

#pragma pack(push, 1)
struct DoseJournalRecordV1 {
    uint32_t epoch{0}; // local-flavored unixtime (DateTime::current().unixtime())
    uint32_t deviceId{0};
    uint8_t type{static_cast<uint8_t>(DoseJournalEntryType::Schedule)};
    uint8_t flags{0}; // reserved
    uint16_t amountCentiMl{0};
};
#pragma pack(pop)

// Persistent append-only dose log. Implementations bound total size themselves (ring semantics);
// append may silently drop the write when storage is unavailable - dosing must never fail because
// the journal cannot be written.
class IDoseJournal {
public:
    // Return false from the visitor to stop the iteration early.
    using Visitor = bool (*)(const DoseJournalRecordV1& record, void* context);

    IDoseJournal() = default;
    IDoseJournal(const IDoseJournal&) = delete;
    IDoseJournal& operator=(const IDoseJournal&) = delete;
    virtual ~IDoseJournal() = default;

    virtual bool append(const DoseJournalRecordV1& record) = 0;
    // Newest-first visitation of records with epoch >= sinceEpoch. deviceId 0 visits all devices
    // sequentially (newest-first within each device, not globally merged across devices).
    virtual void forEachNewestFirst(uint32_t deviceId, uint32_t sinceEpoch, Visitor visitor, void* context) const = 0;
    // Deletes the device's entire history; called when the device is removed from the registry.
    virtual bool removeDevice(uint32_t deviceId) = 0;
};

// Process-wide journal seam (mirrors defaultArduinoGpioOutputDriver()'s singleton-accessor
// pattern, but settable so App wires the LittleFS-backed journal in and tests inject a memory
// one). Null until set - callers must tolerate a missing journal.
IDoseJournal* defaultDoseJournal();
void setDefaultDoseJournal(IDoseJournal* journal);

} // namespace ewfm
