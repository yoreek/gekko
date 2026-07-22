#include "devices/analog/scheduled/presets/SchedulePreset.h"

#include <cstring>
#include <type_traits>

namespace ewfm {

static_assert(std::is_trivially_copyable<SchedulePresetRecordV1>::value, "SchedulePresetRecordV1 must be POD");

namespace {
ISchedulePresetStorage* g_defaultSchedulePresetStorage = nullptr;
} // namespace

void buildSchedulePresetRecord(SchedulePresetRecordV1& out, const char* name,
                               const ScheduledAnalogOutputPointV1 (&points)[kMaxScheduledAnalogOutputPoints]) {
    out = SchedulePresetRecordV1{};
    std::memcpy(out.magic, SchedulePresetRecordV1::kMagic, sizeof(out.magic));
    if (name != nullptr) {
        std::strncpy(out.name, name, kMaxSchedulePresetNameLength);
        out.name[kMaxSchedulePresetNameLength] = '\0';
    }
    std::memcpy(out.points, points, sizeof(out.points));
}

bool schedulePresetRecordValid(const SchedulePresetRecordV1& record) {
    return std::memcmp(record.magic, SchedulePresetRecordV1::kMagic, sizeof(record.magic)) == 0;
}

ISchedulePresetStorage* defaultSchedulePresetStorage() {
    return g_defaultSchedulePresetStorage;
}

void setDefaultSchedulePresetStorage(ISchedulePresetStorage* storage) {
    g_defaultSchedulePresetStorage = storage;
}

} // namespace ewfm
