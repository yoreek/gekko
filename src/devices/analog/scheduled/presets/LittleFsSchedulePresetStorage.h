#pragma once

#include "devices/analog/scheduled/presets/SchedulePreset.h"

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
    // Mounts the devdata partition (formatting on first use) and creates the presets directory.
    // Safe to call after another feature already mounted the same partition.
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
    // FS::open/exists are non-const in the Arduino API; the reads really are logically const.
    mutable fs::LittleFSFS fs_;
#endif
};

} // namespace ewfm
