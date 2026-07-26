#pragma once

#include "metrics/MetricValueResolver.h"

#include <cstdint>

namespace ewfm {

// Role hook for character-cell displays (HD44780-style), parallel to DisplayDeviceBase for
// pixel-graphics displays. Deliberately a plain interface, not a DeviceRuntimeBase subclass: the
// concrete device already derives from DeviceRuntimeBase for its own lifecycle and adds this
// interface alongside it. DisplayRenderCoordinator calls renderText() once per tick, the same way
// it calls DisplayDeviceBase::renderDisplay() for pixel displays.
class CharacterDisplayRuntimeBase {
public:
    virtual ~CharacterDisplayRuntimeBase() = default;

    virtual bool renderText(const MetricValueResolver& resolver, uint32_t now) = 0;
};

} // namespace ewfm
