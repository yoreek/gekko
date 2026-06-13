#pragma once

#include "portal/DashboardLayoutStore.h"
#include "portal/controllers/BaseController.h"

#if defined(ARDUINO) && !defined(UNIT_TEST)
class AsyncWebServer;
class AsyncWebServerRequest;
#endif

namespace ewfm {

class DashboardLayoutController : public BaseController {
public:
    DashboardLayoutController(AsyncWebServerRequest* request, Action action, DashboardLayoutStore& store)
        : BaseController(request, action), store_(store) {}

#if defined(ARDUINO) && !defined(UNIT_TEST)
    static void registerRoutes(AsyncWebServer& server, DashboardLayoutStore& store);
#endif

protected:
    const RulesChain* beforeChain() override;
    void show() override;
    void update() override;
    void options() override;

private:
    static bool parseUpdateBody(BaseController& self);

    DashboardLayoutStore& store_;
};

} // namespace ewfm
