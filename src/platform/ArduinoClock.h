#pragma once

#include "core/Clock.h"

namespace ewfm {

class ArduinoClock final : public IClock {
public:
    uint32_t millis() const override;
};

} // namespace ewfm
