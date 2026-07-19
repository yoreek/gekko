#pragma once

#include "devices/dosing/journal/DoseJournal.h"
#include "integrations/common/DeviceEventBus.h"

namespace ewfm {

// Purges a device's journal history when the device is deleted from the registry, so orphaned
// per-device directories cannot accumulate on the devdata partition. Type-agnostic on purpose:
// removing a device that never journaled is a cheap no-op.
class DoseJournalCleanupSink final : public IDeviceEventSink {
public:
    explicit DoseJournalCleanupSink(IDoseJournal& journal) : journal_(journal) {}

    void onDeviceEvent(const DeviceEvent& event) override {
        if (event.kind == DeviceEventKind::DeviceDeleted) {
            (void)journal_.removeDevice(event.deviceId);
        }
    }

    void tickFastLoop(uint32_t) override {}
    void tick100ms(uint32_t) override {}
    void tick1s(uint32_t) override {}

private:
    IDoseJournal& journal_;
};

} // namespace ewfm
