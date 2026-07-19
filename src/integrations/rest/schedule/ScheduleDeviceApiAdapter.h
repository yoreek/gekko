#pragma once

#include "devices/schedule/ScheduleDevice.h"
#include "integrations/rest/common/TypedDeviceApiAdapter.h"

namespace ewfm {

class ScheduleDeviceApiAdapter final : public TypedDeviceApiAdapter<ScheduleDeviceApiAdapter, ScheduleDevice, ScheduleDeviceConfigV1> {
public:
    static constexpr const char* kTypeName = "schedule";

    // Deliberately no "active"/"timeValid" in the runtime JSON: isActive() is never pushed live
    // (ScheduleDevice never marks itself runtime-dirty), so exposing it over REST/WS would just
    // be a stale snapshot from whenever the page last fetched - misleading rather than useful.
    // The frontend computes an on/off preview client-side from the rule config instead (see
    // portal-spa/src/models/devices/schedule-preview.ts). isActive()/timeValid() are still used
    // internally by AutoSwitchDevice, which re-evaluates them itself every tick.
};

} // namespace ewfm
