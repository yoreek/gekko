#pragma once

#include "devices/switch/SwitchDeviceBase.h"

namespace ewfm {

class TriStateSwitchDeviceBase : public SwitchDeviceBase {
protected:
    explicit TriStateSwitchDeviceBase(const SwitchDeviceConfigV1& config);

    OutputStateMask supportedOutputStateMask() const override;
};

} // namespace ewfm
