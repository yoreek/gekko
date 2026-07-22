#pragma once

#include "devices/analog/scheduled/ScheduledAnalogOutputDeviceConfig.h"

#include <cstddef>
#include <cstdint>

namespace ewfm {

// Named schedule presets ("profiles") for scheduled_analog_output, mainly for aquarium light
// setups. A preset is a full snapshot of one channel's schedule points; each device keeps up to
// kMaxSchedulePresets named slots. Presets live on the devdata LittleFS partition (not in the
// device config blob, which is bounded to kMaxDeviceConfigBytes), one directory per feature like
// the dose journal.
constexpr uint8_t kMaxSchedulePresets = 3U;
constexpr size_t kMaxSchedulePresetNameLength = 32U;

#pragma pack(push, 1)
// On-disk preset record: a trivially-copyable POD read/written as one raw block. The leading
// magic is validated on load so a truncated or foreign file is rejected rather than misread.
struct SchedulePresetRecordV1 {
    static constexpr char kMagic[] = "SAPRE-1";
    char magic[sizeof(kMagic)]{};
    char name[kMaxSchedulePresetNameLength + 1]{};
    ScheduledAnalogOutputPointV1 points[kMaxScheduledAnalogOutputPoints]{};
};
#pragma pack(pop)

// Stamp the magic and copy name/points into a fresh record ready to persist. `name` is truncated
// to kMaxSchedulePresetNameLength and always null-terminated.
void buildSchedulePresetRecord(SchedulePresetRecordV1& out, const char* name,
                               const ScheduledAnalogOutputPointV1 (&points)[kMaxScheduledAnalogOutputPoints]);
bool schedulePresetRecordValid(const SchedulePresetRecordV1& record);

// Persistent storage for schedule presets. Slots are 0..kMaxSchedulePresets-1; load() returns
// false for an empty or invalid slot. One implementation over LittleFS plus a native no-op stand-in.
class ISchedulePresetStorage {
public:
    virtual ~ISchedulePresetStorage() = default;
    virtual bool save(uint32_t deviceId, uint8_t slot, const SchedulePresetRecordV1& record) = 0;
    virtual bool load(uint32_t deviceId, uint8_t slot, SchedulePresetRecordV1& out) const = 0;
    virtual bool erase(uint32_t deviceId, uint8_t slot) = 0;
    virtual bool removeDevice(uint32_t deviceId) = 0;
};

// Process-wide default, wired by App so PortalServer route registration needs no extra ctor args
// (mirrors defaultDoseJournal()).
ISchedulePresetStorage* defaultSchedulePresetStorage();
void setDefaultSchedulePresetStorage(ISchedulePresetStorage* storage);

} // namespace ewfm
