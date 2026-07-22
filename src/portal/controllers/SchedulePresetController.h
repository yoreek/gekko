#pragma once

#include "devices/analog/scheduled/presets/SchedulePreset.h"
#include "portal/controllers/BaseController.h"

#if defined(ARDUINO) && !defined(UNIT_TEST)
class AsyncWebServer;
class AsyncWebServerRequest;
#endif

namespace ewfm {

// /api/schedulepresets - named schedule presets ("profiles") for scheduled_analog_output devices,
// persisted on the devdata partition.
//   GET    ?deviceId=<id>                  -> list every slot (slot, filled, name, points)
//   POST   {deviceId,slot,name,points:[…]} -> save a schedule snapshot into a slot
//   DELETE ?deviceId=<id>&slot=<n>         -> clear a slot
class SchedulePresetController : public BaseController {
public:
    SchedulePresetController(AsyncWebServerRequest* request, Action action, ISchedulePresetStorage* storage);

#if defined(ARDUINO) && !defined(UNIT_TEST)
    static void registerRoutes(AsyncWebServer& server, ISchedulePresetStorage* storage);
#endif

protected:
    void index() override;
    void create() override;
    void destroy() override;
    void options() override;

private:
    ISchedulePresetStorage* storage_;
};

} // namespace ewfm
