#include "devices/display/DisplayRenderCoordinator.h"

#include "metrics/MetricValueResolver.h"

namespace ewfm {

DisplayRenderCoordinator::DisplayRenderCoordinator(DeviceRegistry& registry, IWifiDriver& wifiDriver)
    : registry_(registry), wifiDriver_(wifiDriver) {}

void DisplayRenderCoordinator::tick(const uint32_t now) {
    MetricValueResolver resolver(&registry_, wifiDriver_, now);
    registry_.forEachRuntime([&](IDeviceRuntime& runtime) {
        DisplayDeviceBase* display = runtime.displayRuntime();
        if (display == nullptr) {
            return;
        }
        (void)display->renderDisplay(resolver, now);
    });
}

} // namespace ewfm
