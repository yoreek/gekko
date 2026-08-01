#pragma once

#include "portal/controllers/BaseController.h"

#if defined(ARDUINO) && !defined(UNIT_TEST)
class AsyncWebServer;
class AsyncWebServerRequest;
#endif

namespace ewfm {

class ConfigStore;

class BoardController : public BaseController {
public:
    BoardController(AsyncWebServerRequest* request, Action action, ConfigStore* configStore);

#if defined(ARDUINO) && !defined(UNIT_TEST)
    static void registerRoutes(AsyncWebServer& server, ConfigStore* configStore);
#endif

protected:
    const RulesChain* beforeChain() override;
    void index() override;  // GET /api/system/board
    void update() override; // PUT /api/system/board
    void options() override;

private:
    static bool parseJsonBody(BaseController& self);

    ConfigStore* configStore_{nullptr};
};

} // namespace ewfm
