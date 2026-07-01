#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>

namespace ewfm {

// GPIO mode constants
enum class GpioMode : uint8_t {
    Input = 0,
    Output = 1,
    InputPullup = 2,
    InputPulldown = 3,
};

// State struct for saving/restoring CS pin configuration
struct CsPinState {
    uint8_t pin{0};
    GpioMode mode{GpioMode::Input};
    bool level{false};
};

class ISpiCsProbeDriver {
public:
    ISpiCsProbeDriver() = default;
    ISpiCsProbeDriver(const ISpiCsProbeDriver&) = delete;
    ISpiCsProbeDriver& operator=(const ISpiCsProbeDriver&) = delete;
    ISpiCsProbeDriver(ISpiCsProbeDriver&&) = delete;
    ISpiCsProbeDriver& operator=(ISpiCsProbeDriver&&) = delete;
    virtual ~ISpiCsProbeDriver() = default;

    // Read current pin state (mode and level)
    virtual bool readCurrentState(uint8_t pin, GpioMode& mode, bool& level) = 0;

    // Configure pin as OUTPUT and set initial level
    virtual bool configureOutput(uint8_t pin, bool initialLevel) = 0;

    // Configure pin as INPUT_PULLUP and read resulting level
    virtual bool configureInputPullup(uint8_t pin, bool& level) = 0;

    // Configure pin as INPUT_PULLDOWN and read resulting level
    virtual bool configureInputPulldown(uint8_t pin, bool& level) = 0;

    // Write level to OUTPUT pin
    virtual bool writeLevel(uint8_t pin, bool high) = 0;

    // Read current level (for OUTPUT or INPUT modes)
    virtual bool readLevel(uint8_t pin, bool& level) = 0;

    // Restore pin to a saved state
    virtual bool restoreState(uint8_t pin, GpioMode mode, bool level) = 0;

    // Release/cleanup pin (set to safe INPUT state)
    virtual bool release(uint8_t pin) = 0;
};

ISpiCsProbeDriver& defaultArduinoSpiCsProbeDriver();
std::unique_ptr<ISpiCsProbeDriver> createArduinoSpiCsProbeDriver();

} // namespace ewfm
