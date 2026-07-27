#include "integrations/rest/common/RtcDeviceApiSupport.h"

namespace ewfm {

DeviceValidationResult validateAtMostOneActiveRtcSync(const DeviceRegistry& registry, const IDeviceRuntime* self, bool requestedActive) {
    if (!requestedActive) {
        return {};
    }
    DeviceValidationResult result{};
    registry.forEachRuntime([&](const IDeviceRuntime& runtime) {
        if (!result.ok() || &runtime == self) {
            return;
        }
        const IRealTimeClockRuntime* rtc = runtime.realTimeClockRuntime();
        if (rtc != nullptr && rtc->useForSystemTimeSync()) {
            result = {DeviceError::InvalidConfig, "another RTC device is already set to sync system time"};
        }
    });
    return result;
}

} // namespace ewfm
