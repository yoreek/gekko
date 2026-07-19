#pragma once

#include "devices/display/DisplayDeviceBase.h"
#include "time/DateTime.h"

namespace ewfm {

class IWifiDriver;

class DisplayRenderCoordinator {
public:
    DisplayRenderCoordinator(DeviceRegistry& registry, IWifiDriver& wifiDriver);

    void tick(uint32_t now);

private:
    DeviceRegistry& registry_;
    IWifiDriver& wifiDriver_;
#if defined(ARDUINO) && !defined(UNIT_TEST)
    uint32_t lastLocalTimeSampleMs_{0};
    DateTime cachedLocalTime_{};
    bool hasCachedLocalTime_{false};
#endif
};

} // namespace ewfm
