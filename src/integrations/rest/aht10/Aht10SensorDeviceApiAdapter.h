#pragma once

#include "devices/sensors/aht10/Aht10SensorDevice.h"
#include "integrations/rest/common/TypedDeviceApiAdapter.h"

namespace ewfm {

class Aht10SensorDeviceApiAdapter final
    : public TypedDeviceApiAdapter<Aht10SensorDeviceApiAdapter, Aht10SensorDevice, Aht10SensorConfigV1> {
public:
    static constexpr const char* kTypeName = "aht10";

    bool parseCreateExtras(const JsonObjectConst& input, const JsonObjectConst& configInput, Aht10SensorConfigV1& config,
                           DeviceCreateRequest& request, const char*& error) const;
    bool parseUpdateExtras(const JsonObjectConst& input, const JsonObjectConst& configInput, Aht10SensorConfigV1& config,
                           DeviceConfigUpdateRequest& request, const char*& error) const;
    void writeRuntimeJson(const Aht10SensorDevice& device, JsonObject runtimeJson) const;

    DeviceValidationResult validateCreateRequest(const DeviceCreateRequest& request, const DeviceRegistry& registry) const override;
    DeviceValidationResult validateUpdateConfigRequest(const IDeviceRuntime& runtime, const DeviceConfigUpdateRequest& request,
                                                       const DeviceRegistry& registry) const override;
    DeviceValidationResult validateSetDepsRequest(const IDeviceRuntime& runtime,
                                                  const std::array<DeviceDependencyLink, kMaxDeviceDependencies>& deps, uint8_t depCount,
                                                  const DeviceRegistry& registry) const override;
};

} // namespace ewfm
