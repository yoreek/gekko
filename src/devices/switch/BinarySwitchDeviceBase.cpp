#include "devices/switch/BinarySwitchDeviceBase.h"

namespace ewfm {

BinarySwitchDeviceBase::BinarySwitchDeviceBase(const SwitchDeviceConfigV1& config) : SwitchDeviceBase(config) {}

OutputStateMask BinarySwitchDeviceBase::supportedOutputStateMaskImpl() const {
    return kOutputStateMaskBinary;
}

} // namespace ewfm
