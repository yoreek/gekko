#pragma once

#include "devices/analog/scheduled/presets/SchedulePreset.h"
#include "platform/DevDataPartition.h"

#if defined(ARDUINO) && !defined(UNIT_TEST)
#include <LittleFS.h>
#endif

namespace ewfm {

// Schedule presets on the shared devdata partition (my_partitions.csv), separate from the UI
// asset partition so `pio run -t uploadfs` cannot wipe them. Layout convention (matching the dose
// journal's "/dj"): this feature owns "/sap", one subdirectory per device id underneath, one file
// per preset slot. Device ids are registry-unique, so directory names cannot collide.
class LittleFsSchedulePresetStorage final : public ISchedulePresetStorage {
public:
#if defined(ARDUINO) && !defined(UNIT_TEST)
    // Injected by App, which owns the single mount of the shared devdata partition.
    explicit LittleFsSchedulePresetStorage(fs::LittleFSFS& fs) : fs_(fs) {}
#endif

    // Assumes App has already mounted the devdata partition; creates the presets directory if
    // missing.
    bool begin();

    bool save(uint32_t deviceId, uint8_t slot, const SchedulePresetRecordV1& record) override;
    bool load(uint32_t deviceId, uint8_t slot, SchedulePresetRecordV1& out) const override;
    bool erase(uint32_t deviceId, uint8_t slot) override;
    bool removeDevice(uint32_t deviceId) override;

private:
    static constexpr size_t kMaxPathBytes = 40;
    // LittleFS needs spare blocks for wear-leveled writes; refuse to save rather than run the
    // filesystem down to zero free blocks.
    static constexpr size_t kSchedulePresetMinFreeBytes = 8192;
    static void buildDevicePath(char (&out)[kMaxPathBytes], uint32_t deviceId);
    static void buildSlotPath(char (&out)[kMaxPathBytes], uint32_t deviceId, uint8_t slot);

#if defined(ARDUINO) && !defined(UNIT_TEST)
    fs::LittleFSFS& fs_;
#endif
};

} // namespace ewfm
