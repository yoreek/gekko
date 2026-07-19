#pragma once

#include "devices/bus/i2c/I2cBusDevice.h"
#include "integrations/rest/common/TypedDeviceApiAdapter.h"

namespace ewfm {

class I2cBusDeviceApiAdapter final : public TypedDeviceApiAdapter<I2cBusDeviceApiAdapter, I2cBusDevice, I2cBusDeviceConfigV1> {
public:
    static constexpr const char* kTypeName = "i2c_bus";

    void writeRuntimeJson(const I2cBusDevice& device, JsonObject runtimeJson) const;
};

} // namespace ewfm
