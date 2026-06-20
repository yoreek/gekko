#pragma once

#include "devices/switch/SwitchDeviceBase.h"

namespace ewfm {

class BinarySwitchDeviceBase : public SwitchDeviceBase {
protected:
    explicit BinarySwitchDeviceBase(const SwitchDeviceConfigV1& config);

    OutputStateMask supportedOutputStateMaskImpl() const override;
};

} // namespace ewfm
