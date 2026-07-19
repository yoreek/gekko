#pragma once

#include "config/DeviceConfig.h"
#include "portal/controllers/BaseController.h"

#if defined(ARDUINO) && !defined(UNIT_TEST)
class AsyncWebServer;
class AsyncWebServerRequest;
#endif

namespace ewfm {

class NtpManager;

class TimeController : public BaseController {
public:
    TimeController(AsyncWebServerRequest* request, Action action, NtpManager* ntpManager);

#if defined(ARDUINO) && !defined(UNIT_TEST)
    static void registerRoutes(AsyncWebServer& server, NtpManager* ntpManager);
#endif

protected:
    const RulesChain* beforeChain() override;
    void index() override;  // GET /api/system/time/settings
    void show() override;   // GET /api/system/time
    void create() override; // POST /api/system/time (manual time set)
    void update() override; // PUT /api/system/time/settings
    void options() override;

private:
    static bool parseJsonBody(BaseController& self);
    static const char* errorCodeForTimeError(ConfigError error);

    NtpManager* ntpManager_{nullptr};
};

} // namespace ewfm
