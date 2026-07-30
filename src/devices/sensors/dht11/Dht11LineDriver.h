#pragma once

#include <cstdint>

namespace ewfm {

class IDht11LineDriver {
public:
    IDht11LineDriver() = default;
    IDht11LineDriver(const IDht11LineDriver&) = delete;
    IDht11LineDriver& operator=(const IDht11LineDriver&) = delete;
    virtual ~IDht11LineDriver() = default;

    virtual bool driveLow(uint8_t pin) = 0;
    virtual bool release(uint8_t pin, bool internalPullup) = 0;
    virtual bool read(uint8_t pin, bool& level) = 0;
    virtual void waitMicros(uint32_t microseconds) = 0;
};

class ArduinoDht11LineDriver final : public IDht11LineDriver {
public:
    bool driveLow(uint8_t pin) override;
    bool release(uint8_t pin, bool internalPullup) override;
    bool read(uint8_t pin, bool& level) override;
    void waitMicros(uint32_t microseconds) override;
};

ArduinoDht11LineDriver& defaultArduinoDht11LineDriver();

} // namespace ewfm
