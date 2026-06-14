#pragma once

#include "devices/core/DeviceTypes.h"
#include "devices/registry/DeviceRegistry.h"

#include <ArduinoJson.h>
#include <string>
#include <vector>

namespace ewfm {

class IDeviceApiAdapter {
public:
    IDeviceApiAdapter() = default;
    IDeviceApiAdapter(const IDeviceApiAdapter&) = delete;
    IDeviceApiAdapter& operator=(const IDeviceApiAdapter&) = delete;
    IDeviceApiAdapter(IDeviceApiAdapter&&) = delete;
    IDeviceApiAdapter& operator=(IDeviceApiAdapter&&) = delete;
    virtual ~IDeviceApiAdapter() = default;

    virtual DeviceTypeId typeId() const = 0;
    virtual const char* typeName() const = 0;
    virtual bool parseCreateRequest(const JsonObjectConst& input, DeviceCreateRequest& request, std::string& error) const = 0;
    virtual void writeDeviceJson(const DeviceRecord& record, const IDeviceRuntime* runtime, JsonObject output) const = 0;
};

class DeviceApiAdapterRegistry {
public:
    bool registerAdapter(const IDeviceApiAdapter& adapter);
    const IDeviceApiAdapter* find(DeviceTypeId typeId) const;
    const IDeviceApiAdapter* findByName(const char* name) const;

    static DeviceApiAdapterRegistry withDefaults();

private:
    std::vector<const IDeviceApiAdapter*> adapters_{};
};

} // namespace ewfm
