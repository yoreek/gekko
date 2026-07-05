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
    char name[kMaxDeviceBaseNameLength + 1]{};
    bool enabled{true};
    bool depsProvided{false};
    std::array<DeviceDependencyLink, kMaxDeviceDependencies> deps{};
    uint8_t depCount{0};
    BoundedBlob<kMaxDisplayLayoutBytes> persistedStateBlob{};
    bool persistedStateProvided{false};
};

struct DeviceCreatePersistenceRequest {
    BoundedBlob<kMaxDisplayLayoutBytes> persistedStateBlob{};
    bool persistedStateProvided{false};
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
    virtual bool parseCreatePersistedStateRequest(const JsonObjectConst& input, DeviceCreateRequest& request,
                                                  DeviceCreatePersistenceRequest& persistedRequest, const char*& error) const;
    virtual bool parseUpdateConfigRequest(const JsonObjectConst& input, IDeviceRuntime& runtime, DeviceConfigUpdateRequest& request,
                                          const char*& error) const;
    virtual DeviceValidationResult validateCreateRequest(const DeviceCreateRequest& request, const DeviceRegistry& registry) const;
    virtual DeviceValidationResult validateCreateRequest(const DeviceCreateRequest& request,
                                                         const DeviceCreatePersistenceRequest& persistedRequest,
                                                         const DeviceRegistry& registry) const;
    virtual DeviceValidationResult validateUpdateConfigRequest(const IDeviceRuntime& runtime, const DeviceConfigUpdateRequest& request,
                                                               const DeviceRegistry& registry) const;
    virtual DeviceValidationResult validateSetDepsRequest(const IDeviceRuntime& runtime,
                                                          const std::array<DeviceDependencyLink, kMaxDeviceDependencies>& deps,
                                                          uint8_t depCount, const DeviceRegistry& registry) const;
    virtual void writeDeviceJson(const IDeviceRuntime& runtime, DeviceStatus effectiveStatus, JsonObject output) const = 0;

protected:
    static void writeCommonDeviceJson(const IDeviceRuntime& runtime, DeviceStatus effectiveStatus, const char* typeName, JsonObject output);
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
