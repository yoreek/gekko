#pragma once

#include "integrations/common/DeviceApiAdapter.h"

namespace ewfm {

class Ds18b20TemperatureSensorDeviceApiAdapter final : public IDeviceApiAdapter {
public:
    static const Ds18b20TemperatureSensorDeviceApiAdapter& instance();

    DeviceTypeId typeId() const override;
    const char* typeName() const override;
    bool parseCreateRequest(const JsonObjectConst& input, DeviceCreateRequest& request, const char*& error) const override;
    bool parseUpdateConfigRequest(const JsonObjectConst& input, const IDeviceRuntime& runtime, DeviceConfigUpdateRequest& request,
                                  const char*& error) const override;
    DeviceValidationResult validateCreateRequest(const DeviceCreateRequest& request, const DeviceRegistry& registry) const override;
    DeviceValidationResult validateUpdateConfigRequest(const IDeviceRuntime& runtime, const DeviceConfigUpdateRequest& request,
                                                       const DeviceRegistry& registry) const override;
    DeviceValidationResult validateSetParentRequest(const IDeviceRuntime& runtime, bool hasParent, DeviceId parentDeviceId,
                                                    const DeviceRegistry& registry) const override;
    void writeDeviceJson(const IDeviceRuntime& runtime, JsonObject output) const override;
};

} // namespace ewfm
