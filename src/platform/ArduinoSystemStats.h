#pragma once

#include "core/SystemStats.h"

namespace ewfm {

class ArduinoSystemStats final : public ISystemStats {
public:
    uint32_t freeHeapBytes() const override;
    uint8_t heapFragmentationPercent() const override;
};

} // namespace ewfm
