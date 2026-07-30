#pragma once

#include "devices/bus/i2c/I2cDeviceValidation.h"
#include "devices/display/lcd2004/Lcd2004Device.h"
#include "integrations/rest/display/TypedDisplayDeviceApiAdapter.h"

namespace ewfm {

class Lcd2004DeviceApiAdapter final : public TypedDisplayDeviceApiAdapter<Lcd2004DeviceApiAdapter, Lcd2004Device, Lcd2004DeviceConfigV2> {
public:
    static constexpr const char* kTypeName = "lcd2004";
    static constexpr const char* kBusDependencyError = kI2cBusDependencyRequiredError;

    static const Lcd2004DeviceApiAdapter& instance();
    static bool decodeConfig(const uint8_t* input, size_t size, Lcd2004DeviceConfigV2& config);
    static DeviceRole busRole();
    static DeviceId configBusDeviceId(const Lcd2004DeviceConfigV2& config);
    static void setConfigBusDeviceId(Lcd2004DeviceConfigV2& config, DeviceId busDeviceId);
    static DeviceValidationResult validateBusDependency(const DeviceRegistry& registry, DeviceId busDeviceId,
                                                        const Lcd2004DeviceConfigV2& config, const IDeviceRuntime* ignoreDependent);
};

} // namespace ewfm
