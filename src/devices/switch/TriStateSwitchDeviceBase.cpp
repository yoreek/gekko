#include "devices/switch/TriStateSwitchDeviceBase.h"

namespace ewfm {

TriStateSwitchDeviceBase::TriStateSwitchDeviceBase(const SwitchDeviceConfigV1& config) : SwitchDeviceBase(config) {}

OutputStateMask TriStateSwitchDeviceBase::supportedOutputStateMask() const {
    return kOutputStateMaskTriState;
}

} // namespace ewfm
