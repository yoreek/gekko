#pragma once

#include "devices/registry/DeviceRegistry.h"
#include "integrations/common/DeviceApiAdapter.h"
#include "portal/controllers/BaseController.h"
#include "portal/routes/DeviceRegistryRouteParser.h"

#if defined(ARDUINO) && !defined(UNIT_TEST)
class AsyncWebServer;
class AsyncWebServerRequest;
#endif

namespace ewfm {

class DeviceRegistryController : public BaseController {
public:
    DeviceRegistryController(AsyncWebServerRequest* request, Action action, DeviceRegistry& registry,
                             const DeviceApiAdapterRegistry& adapters);

#if defined(ARDUINO) && !defined(UNIT_TEST)
    static void registerRoutes(AsyncWebServer& server, DeviceRegistry& registry);
#endif

protected:
    void index() override;
    void show() override;
    void create() override;
    void destroy() override;
    void cmd() override;
    void flush() override;
    void options() override;

private:
    DeviceRegistry& registry_;
    DeviceRegistryRouteParser parser_;
    const DeviceApiAdapterRegistry& adapters_;

    bool parseDeviceId(bool allowCommandSuffix, DeviceId& deviceId) const;
    bool parseCreateAdapter(const JsonVariantConst& json, std::string& error, const IDeviceApiAdapter*& adapter) const;
    static const char* statusToString(DeviceStatus status);
    static const char* errorCodeForDeviceError(DeviceError error);
    static DevicePersistencePolicy parsePolicy(const JsonObjectConst& input);
    static bool parseCommandType(const char* value, DeviceCommandType& type);
};

} // namespace ewfm
