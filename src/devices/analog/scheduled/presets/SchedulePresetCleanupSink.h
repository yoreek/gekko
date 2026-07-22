#pragma once

#include "devices/analog/scheduled/presets/SchedulePreset.h"
#include "integrations/common/DeviceEventBus.h"

namespace ewfm {

// Purges a device's schedule presets when it is deleted from the registry, so orphaned per-device
// directories cannot accumulate on the devdata partition. Type-agnostic on purpose: removing a
// device that never saved a preset is a cheap no-op.
class SchedulePresetCleanupSink final : public IDeviceEventSink {
public:
    explicit SchedulePresetCleanupSink(ISchedulePresetStorage& storage) : storage_(storage) {}

    void onDeviceEvent(const DeviceEvent& event) override {
        if (event.kind == DeviceEventKind::DeviceDeleted) {
            (void)storage_.removeDevice(event.deviceId);
        }
    }

    void tickFastLoop(uint32_t) override {}
    void tick100ms(uint32_t) override {}
    void tick1s(uint32_t) override {}

private:
    ISchedulePresetStorage& storage_;
};

} // namespace ewfm
