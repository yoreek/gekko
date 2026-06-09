#pragma once

#include "devices/registry/DeviceRegistry.h"
#include "integrations/common/DeviceApiAdapter.h"

#if defined(ARDUINO) && !defined(UNIT_TEST)
#include <ArduinoJson.h>

class AsyncWebServerRequest;
#endif

namespace ewfm {

class DeviceRegistryRouteParser {
public:
    DeviceRegistryRouteParser(DeviceRegistry& registry, const DeviceApiAdapterRegistry& adapters);

#if defined(ARDUINO) && !defined(UNIT_TEST)
    bool parseDeviceId(const AsyncWebServerRequest* request, DeviceId& deviceId) const;
    const DeviceRecord* findRecord(const AsyncWebServerRequest* request) const;
    const IDeviceApiAdapter* findAdapterForRecord(const DeviceRecord& record) const;
    const IDeviceApiAdapter* findAdapterForCreate(const JsonVariant& json, std::string& error) const;
#endif

private:
    DeviceRegistry& registry_;
    const DeviceApiAdapterRegistry& adapters_;
};

} // namespace ewfm
