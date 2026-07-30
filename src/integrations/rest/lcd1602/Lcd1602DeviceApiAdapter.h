#pragma once

#include "devices/bus/i2c/I2cDeviceValidation.h"
#include "devices/display/lcd1602/Lcd1602Device.h"
#include "integrations/rest/display/TypedDisplayDeviceApiAdapter.h"

namespace ewfm {

class Lcd1602DeviceApiAdapter final : public TypedDisplayDeviceApiAdapter<Lcd1602DeviceApiAdapter, Lcd1602Device, Lcd1602DeviceConfigV2> {
public:
    static constexpr const char* kTypeName = "lcd1602";
    static constexpr const char* kBusDependencyError = kI2cBusDependencyRequiredError;

    static const Lcd1602DeviceApiAdapter& instance();
    static bool decodeConfig(const uint8_t* input, size_t size, Lcd1602DeviceConfigV2& config);
    static DeviceRole busRole();
    static DeviceId configBusDeviceId(const Lcd1602DeviceConfigV2& config);
    static void setConfigBusDeviceId(Lcd1602DeviceConfigV2& config, DeviceId busDeviceId);
    static DeviceValidationResult validateBusDependency(const DeviceRegistry& registry, DeviceId busDeviceId,
                                                        const Lcd1602DeviceConfigV2& config, const IDeviceRuntime* ignoreDependent);
};

} // namespace ewfm
