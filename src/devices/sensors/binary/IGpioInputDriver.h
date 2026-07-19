#pragma once

#include <cstdint>

namespace ewfm {

enum class GpioInputPullMode : uint8_t {
    None = 0,
    PullUp = 1,
    PullDown = 2,
};

class IGpioInputDriver {
public:
    IGpioInputDriver() = default;
    IGpioInputDriver(const IGpioInputDriver&) = delete;
    IGpioInputDriver& operator=(const IGpioInputDriver&) = delete;
    virtual ~IGpioInputDriver() = default;

    virtual bool configureInput(uint8_t pin, GpioInputPullMode pullMode) = 0;
    virtual bool read(uint8_t pin, bool& level) = 0;
    virtual void release(uint8_t pin) = 0;
};

} // namespace ewfm
