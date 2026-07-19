#pragma once

#include "config/MqttConfigStore.h"
#include "platform/MqttManager.h"
#include "portal/controllers/BaseController.h"

#if defined(ARDUINO) && !defined(UNIT_TEST)
class AsyncWebServer;
class AsyncWebServerRequest;
#endif

namespace ewfm {

class HaDiscoveryBridge;

class MqttController : public BaseController {
public:
    MqttController(AsyncWebServerRequest* request, Action action, MqttConfigStore* store, MqttManager* manager,
                   HaDiscoveryBridge* haDiscoveryBridge = nullptr);

    // Reflects whether WITH_HOME_ASSISTANT was compiled in - this is the field the frontend keys
    // off of to gray out the settings page, mirroring OtaStatusResponse.enabled's convention.
    static constexpr bool compiledIn() {
#if defined(WITH_HOME_ASSISTANT)
        return true;
#else
        return false;
#endif
    }

#if defined(ARDUINO) && !defined(UNIT_TEST)
    static void registerRoutes(AsyncWebServer& server, MqttConfigStore* store, MqttManager* manager,
                               HaDiscoveryBridge* haDiscoveryBridge = nullptr);
#endif

protected:
    const RulesChain* beforeChain() override;
    void index() override;   // GET /api/mqtt/settings
    void show() override;    // GET /api/mqtt/status
    void update() override;  // PUT /api/mqtt/settings
    void create() override;  // POST /api/mqtt/ca-cert
    void destroy() override; // DELETE /api/mqtt/ca-cert
    void options() override;

private:
    static bool parseUpdateBody(BaseController& self);
    static const char* errorCodeForMqttError(MqttConfigError error);

    MqttConfigStore* store_{nullptr};
    MqttManager* manager_{nullptr};
    HaDiscoveryBridge* haDiscoveryBridge_{nullptr};
};

} // namespace ewfm
