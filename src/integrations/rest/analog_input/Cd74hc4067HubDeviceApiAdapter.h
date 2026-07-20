#pragma once

#include "devices/analog/input/cd74hc4067/Cd74hc4067HubDevice.h"
#include "integrations/rest/common/TypedDeviceApiAdapter.h"

namespace ewfm {

class Cd74hc4067HubDeviceApiAdapter final
    : public TypedDeviceApiAdapter<Cd74hc4067HubDeviceApiAdapter, Cd74hc4067HubDevice, Cd74hc4067HubDeviceConfigV1> {
public:
    static constexpr const char* kTypeName = "cd74hc4067_hub";
};

} // namespace ewfm
