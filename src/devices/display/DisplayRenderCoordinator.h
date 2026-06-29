#pragma once

#include "devices/display/DisplayDeviceBase.h"

namespace ewfm {

class IWifiDriver;

class DisplayRenderCoordinator {
public:
    DisplayRenderCoordinator(DeviceRegistry& registry, IWifiDriver& wifiDriver);

    void tick(uint32_t now);

private:
    DeviceRegistry& registry_;
    IWifiDriver& wifiDriver_;
};

} // namespace ewfm
