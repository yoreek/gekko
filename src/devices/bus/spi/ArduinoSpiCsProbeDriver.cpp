#include "devices/bus/spi/ISpiCsProbeDriver.h"

#include <memory>

#if defined(ARDUINO) && !defined(UNIT_TEST)
#include <Arduino.h>

namespace ewfm {

class ArduinoSpiCsProbeDriver final : public ISpiCsProbeDriver {
public:
    bool readCurrentState(uint8_t pin, GpioMode& mode, bool& level) override {
        // Note: Arduino doesn't provide API to read pin mode reliably.
        // We store mode separately when pin is configured.
        // For now, read the current level and assume OUTPUT unless explicitly set.
        level = digitalRead(pin) != 0;
        mode = GpioMode::Output; // Safe assumption for CS pins (typically OUTPUT)
        return true;
    }

    bool configureOutput(uint8_t pin, bool initialLevel) override {
        pinMode(pin, OUTPUT);
        digitalWrite(pin, initialLevel ? HIGH : LOW);
        return true;
    }

    bool configureInputPullup(uint8_t pin, bool& level) override {
        pinMode(pin, INPUT_PULLUP);
        delayMicroseconds(10); // Allow line to settle
        level = digitalRead(pin) != 0;
        return true;
    }

    bool configureInputPulldown(uint8_t pin, bool& level) override {
        pinMode(pin, INPUT_PULLDOWN);
        delayMicroseconds(10); // Allow line to settle
        level = digitalRead(pin) != 0;
        return true;
    }

    bool writeLevel(uint8_t pin, bool high) override {
        digitalWrite(pin, high ? HIGH : LOW);
        return true;
    }

    bool readLevel(uint8_t pin, bool& level) override {
        level = digitalRead(pin) != 0;
        return true;
    }

    bool restoreState(uint8_t pin, GpioMode mode, bool level) override {
        switch (mode) {
        case GpioMode::Output:
            pinMode(pin, OUTPUT);
            digitalWrite(pin, level ? HIGH : LOW);
            break;
        case GpioMode::Input:
            pinMode(pin, INPUT);
            break;
        case GpioMode::InputPullup:
            pinMode(pin, INPUT_PULLUP);
            break;
        case GpioMode::InputPulldown:
            pinMode(pin, INPUT_PULLDOWN);
            break;
        }
        return true;
    }

    bool release(uint8_t pin) override {
        pinMode(pin, INPUT);
        return true;
    }
};

ISpiCsProbeDriver& defaultArduinoSpiCsProbeDriver() {
    static ArduinoSpiCsProbeDriver driver;
    return driver;
}

std::unique_ptr<ISpiCsProbeDriver> createArduinoSpiCsProbeDriver() {
    return std::unique_ptr<ISpiCsProbeDriver>(new ArduinoSpiCsProbeDriver());
}

} // namespace ewfm

#else

namespace ewfm {

class NullSpiCsProbeDriver final : public ISpiCsProbeDriver {
public:
    bool readCurrentState(uint8_t, GpioMode& mode, bool& level) override {
        mode = GpioMode::Input;
        level = false;
        return true;
    }

    bool configureOutput(uint8_t, bool) override {
        return true;
    }
    bool configureInputPullup(uint8_t, bool& level) override {
        level = false;
        return true;
    }
    bool configureInputPulldown(uint8_t, bool& level) override {
        level = false;
        return true;
    }
    bool writeLevel(uint8_t, bool) override {
        return true;
    }
    bool readLevel(uint8_t, bool& level) override {
        level = false;
        return true;
    }
    bool restoreState(uint8_t, GpioMode, bool) override {
        return true;
    }
    bool release(uint8_t) override {
        return true;
    }
};

ISpiCsProbeDriver& defaultArduinoSpiCsProbeDriver() {
    static NullSpiCsProbeDriver driver;
    return driver;
}

std::unique_ptr<ISpiCsProbeDriver> createArduinoSpiCsProbeDriver() {
    return std::unique_ptr<ISpiCsProbeDriver>(new NullSpiCsProbeDriver());
}

} // namespace ewfm

#endif
