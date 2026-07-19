#pragma once

#include "devices/registry/DeviceRegistry.h"
#include "integrations/common/DeviceApiAdapter.h"
#include "portal/controllers/BaseController.h"

#if defined(WITH_HOME_ASSISTANT)
#include "integrations/mqtt/HaEntityAdapter.h"
#endif

#if defined(ARDUINO) && !defined(UNIT_TEST)
class AsyncWebServer;
class AsyncWebServerRequest;
#endif

namespace ewfm {

class DeviceScopedDataStore;
class HaDiscoveryBridge;

class DeviceRegistryController : public BaseController {
public:
    DeviceRegistryController(AsyncWebServerRequest* request, Action action, DeviceRegistry& registry,
                             const DeviceApiAdapterRegistry& adapters, DeviceScopedDataStore* haSettingsStore = nullptr,
                             HaDiscoveryBridge* haDiscoveryBridge = nullptr);

#if defined(ARDUINO) && !defined(UNIT_TEST)
    static void registerRoutes(AsyncWebServer& server, DeviceRegistry& registry, DeviceScopedDataStore* haSettingsStore = nullptr,
                               HaDiscoveryBridge* haDiscoveryBridge = nullptr);
#endif

#if defined(UNIT_TEST)
    static bool parseDeviceIdPathForTest(const char* url, bool requireCommandSuffix, DeviceId& deviceId) {
        return parseDeviceIdPath(url, requireCommandSuffix, deviceId);
    }
    static bool parseLayoutPathForTest(const char* url, DeviceId& deviceId) {
        return parseLayoutPath(url, deviceId);
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
    DeviceScopedDataStore* haSettingsStore_{nullptr};
    HaDiscoveryBridge* haDiscoveryBridge_{nullptr};
#if defined(WITH_HOME_ASSISTANT)
    HaEntityAdapterRegistry haAdapters_{HaEntityAdapterRegistry::withDefaults()};
#endif
    DeviceId deviceId_{0};
    bool layoutRequested_{false}; // GET /api/devices/:id/layout matched, show() streams the layout

    static bool parseDeviceIdPath(const char* url, bool requireCommandSuffix, DeviceId& deviceId);
    static bool parseLayoutPath(const char* url, DeviceId& deviceId);
    static bool requireId(BaseController& self);
    static bool requireEntity(BaseController& self);
#if defined(ARDUINO) && !defined(UNIT_TEST)
    void showLayout(const IDeviceRuntime& runtime);
#endif
    bool parseCreateAdapter(const JsonVariantConst& json, const char*& error, const IDeviceApiAdapter*& adapter) const;
    static const char* statusToString(DeviceStatus status);
    static const char* errorCodeForDeviceError(DeviceError error);
};

} // namespace ewfm
