#include "devices/core/DeviceIdGenerator.h"

#if defined(ARDUINO)
#include <esp_system.h>

namespace ewfm {

bool EspRandomDeviceIdSource::next(DeviceId& out) {
    out = static_cast<DeviceId>(::esp_random());
    return true;
}

} // namespace ewfm
#endif
