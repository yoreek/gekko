#include "devices/display/DisplayRenderCoordinator.h"

#include "metrics/MetricValueResolver.h"

namespace ewfm {

DisplayRenderCoordinator::DisplayRenderCoordinator(DeviceRegistry& registry, IWifiDriver& wifiDriver)
    : registry_(registry), wifiDriver_(wifiDriver) {}

void DisplayRenderCoordinator::tick(const uint32_t now) {
    DateTime localTime;
#if defined(ARDUINO) && !defined(UNIT_TEST)
    if (!hasCachedLocalTime_ || now - lastLocalTimeSampleMs_ >= 1000U) {
        cachedLocalTime_ = DateTime::current();
        lastLocalTimeSampleMs_ = now;
        hasCachedLocalTime_ = true;
    }
    localTime = cachedLocalTime_;
#endif
    MetricValueResolver resolver(&registry_, wifiDriver_, now, localTime);
    registry_.forEachRuntime([&](IDeviceRuntime& runtime) {
        if (DisplayDeviceBase* display = runtime.displayRuntime()) {
            (void)display->renderDisplay(resolver, now);
            return;
        }
    });
}

} // namespace ewfm
