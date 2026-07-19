#pragma once

#include "devices/bus/spi/SpiBusDevice.h"
#include "integrations/rest/common/TypedDeviceApiAdapter.h"

namespace ewfm {

class SpiBusDeviceApiAdapter final : public TypedDeviceApiAdapter<SpiBusDeviceApiAdapter, SpiBusDevice, SpiBusDeviceConfigV1> {
public:
    static constexpr const char* kTypeName = "spi_bus";

    void writeRuntimeJson(const SpiBusDevice& device, JsonObject runtimeJson) const;
};

} // namespace ewfm
