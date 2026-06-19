#pragma once

#include "devices/registry/DeviceRegistry.h"
#include "integrations/common/DeviceApiAdapter.h"
#include "portal/controllers/BaseController.h"

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

#if defined(UNIT_TEST)
    static bool parseDeviceIdPathForTest(const char* url, bool requireCommandSuffix, DeviceId& deviceId) {
        return parseDeviceIdPath(url, requireCommandSuffix, deviceId);
    }
#endif

protected:
    const RulesChain* beforeChain() override;
    void index() override;
    void show() override;
    void create() override;
    void destroy() override;
    void cmd() override;
    void flush() override;
    void options() override;

private:
    DeviceRegistry& registry_;
    const DeviceApiAdapterRegistry& adapters_;
    DeviceId deviceId_{0};
    const DeviceRecord* record_{nullptr};

    static bool parseDeviceIdPath(const char* url, bool requireCommandSuffix, DeviceId& deviceId);
    static bool requireId(BaseController& self);
    static bool requireEntity(BaseController& self);
    bool parseCreateAdapter(const JsonVariantConst& json, std::string& error, const IDeviceApiAdapter*& adapter) const;
    static const char* statusToString(DeviceStatus status);
    static const char* errorCodeForDeviceError(DeviceError error);
    static DevicePersistencePolicy parsePolicy(const JsonObjectConst& input);
};

} // namespace ewfm
