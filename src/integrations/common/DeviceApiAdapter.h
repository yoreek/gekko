#pragma once

#include "devices/core/DeviceTypes.h"
#include "devices/registry/DeviceRegistry.h"

#include <ArduinoJson.h>
#include <string>
#include <vector>

namespace ewfm {

struct DeviceConfigUpdateRequest {
    BoundedBlob<kMaxDeviceConfigBytes> configBlob{};
    uint32_t configVersion{0};
    bool parentFieldsProvided{false};
    bool hasParent{false};
    DeviceId parentDeviceId{0};
};

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
    virtual bool parseCreateRequest(const JsonObjectConst& input, DeviceCreateRequest& request, const char*& error) const = 0;
    virtual bool parseUpdateConfigRequest(const JsonObjectConst& input, const IDeviceRuntime& runtime, DeviceConfigUpdateRequest& request,
                                          const char*& error) const;
    virtual DeviceValidationResult validateCreateRequest(const DeviceCreateRequest& request, const DeviceRegistry& registry) const;
    virtual DeviceValidationResult validateUpdateConfigRequest(const IDeviceRuntime& runtime, const DeviceConfigUpdateRequest& request,
                                                               const DeviceRegistry& registry) const;
    virtual DeviceValidationResult validateSetParentRequest(const IDeviceRuntime& runtime, bool hasParent, DeviceId parentDeviceId,
                                                            const DeviceRegistry& registry) const;
    virtual void writeDeviceJson(const IDeviceRuntime& runtime, JsonObject output) const = 0;

protected:
    static void writeCommonDeviceJson(const IDeviceRuntime& runtime, const char* typeName, const char* status,
                                      const char* persistencePolicy, bool retainedStateSupported, JsonObject output);
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
