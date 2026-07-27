#pragma once

#include "devices/core/DeviceTypes.h"

#include <cstdint>

namespace ewfm {

class ITm1637LineDriver {
public:
    virtual ~ITm1637LineDriver() = default;
    virtual bool setClock(bool level, uint32_t now) = 0;
    virtual bool setData(bool level, uint32_t now) = 0;
};

class SwitchTm1637LineDriver final : public ITm1637LineDriver {
public:
    SwitchTm1637LineDriver(ISwitchOutputRuntime& clockLine, ISwitchOutputRuntime& dataLine);

    bool setClock(bool level, uint32_t now) override;
    bool setData(bool level, uint32_t now) override;

private:
    ISwitchOutputRuntime& clockLine_;
    ISwitchOutputRuntime& dataLine_;
};

} // namespace ewfm
