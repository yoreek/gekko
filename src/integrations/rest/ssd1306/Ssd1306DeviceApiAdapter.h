#pragma once

#include "devices/display/DisplayLayoutStore.h"
#include "integrations/common/DeviceApiAdapter.h"

namespace ewfm {

class Ssd1306DeviceApiAdapter final : public IDeviceApiAdapter {
public:
    static const Ssd1306DeviceApiAdapter& instance();

    DeviceTypeId typeId() const override;
    const char* typeName() const override;
    bool parseCreateRequest(const JsonObjectConst& input, DeviceCreateRequest& request, const char*& error) const override;
    bool parseCreatePersistedStateRequest(const JsonObjectConst& input, DeviceCreateRequest& request,
                                          DeviceCreatePersistenceRequest& persistedRequest, const char*& error) const override;
    bool parseUpdateConfigRequest(const JsonObjectConst& input, IDeviceRuntime& runtime, DeviceConfigUpdateRequest& request,
                                  const char*& error) const override;
    DeviceValidationResult validateCreateRequest(const DeviceCreateRequest& request, const DeviceRegistry& registry) const override;
    DeviceValidationResult validateCreateRequest(const DeviceCreateRequest& request, const DeviceCreatePersistenceRequest& persistedRequest,
                                                 const DeviceRegistry& registry) const override;
    DeviceValidationResult validateUpdateConfigRequest(const IDeviceRuntime& runtime, const DeviceConfigUpdateRequest& request,
                                                       const DeviceRegistry& registry) const override;
    DeviceValidationResult validateSetDepsRequest(const IDeviceRuntime& runtime,
                                                  const std::array<DeviceDependencyLink, kMaxDeviceDependencies>& deps, uint8_t depCount,
                                                  const DeviceRegistry& registry) const override;
    void writeDeviceJson(const IDeviceRuntime& runtime, DeviceStatus effectiveStatus, JsonObject output) const override;
};

} // namespace ewfm
