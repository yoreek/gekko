#pragma once

#include "devices/bus/i2c/I2cDeviceValidation.h"
#include "devices/display/ssd1306/Ssd1306Device.h"
#include "integrations/rest/display/TypedDisplayDeviceApiAdapter.h"

namespace ewfm {

class Ssd1306DeviceApiAdapter final : public TypedDisplayDeviceApiAdapter<Ssd1306DeviceApiAdapter, Ssd1306Device, Ssd1306DeviceConfigV6> {
public:
    static constexpr const char* kTypeName = "ssd1306";
    static constexpr const char* kBusDependencyError = kI2cBusDependencyRequiredError;

    static const Ssd1306DeviceApiAdapter& instance();
    static bool decodeConfig(const uint8_t* input, size_t size, Ssd1306DeviceConfigV6& config);
    static DeviceRole busRole();
    static DeviceId configBusDeviceId(const Ssd1306DeviceConfigV6& config);
    static void setConfigBusDeviceId(Ssd1306DeviceConfigV6& config, DeviceId busDeviceId);
    static DeviceValidationResult validateBusDependency(const DeviceRegistry& registry, DeviceId busDeviceId,
                                                        const Ssd1306DeviceConfigV6& config, const IDeviceRuntime* ignoreDependent);
};

} // namespace ewfm
