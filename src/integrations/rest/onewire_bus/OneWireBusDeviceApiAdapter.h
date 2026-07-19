#pragma once

#include "devices/bus/onewire/OneWireBusDevice.h"
#include "integrations/rest/common/TypedDeviceApiAdapter.h"

namespace ewfm {

class OneWireBusDeviceApiAdapter final
    : public TypedDeviceApiAdapter<OneWireBusDeviceApiAdapter, OneWireBusDevice, OneWireBusDeviceConfigV1> {
public:
    static constexpr const char* kTypeName = "onewire_bus";

    void writeRuntimeJson(const OneWireBusDevice& device, JsonObject runtimeJson) const;
};

} // namespace ewfm
