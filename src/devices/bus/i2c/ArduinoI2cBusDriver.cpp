#include "devices/bus/i2c/II2cBusDriver.h"

#if defined(ARDUINO) && !defined(UNIT_TEST)
#include <Arduino.h>
#include <Wire.h>
#endif

#include <memory>

namespace ewfm {

#if defined(ARDUINO) && !defined(UNIT_TEST)
class ArduinoI2cBusDriver final : public II2cBusDriver {
public:
    bool begin(uint8_t sdaPin, uint8_t sclPin, uint32_t frequencyHz, bool internalPullup) override {
        wire_.reset(new TwoWire(0));
        if (wire_ == nullptr) {
            return false;
        }
        if (internalPullup) {
            pinMode(sdaPin, INPUT_PULLUP);
            pinMode(sclPin, INPUT_PULLUP);
        }
        return wire_->begin(sdaPin, sclPin, frequencyHz);
    }

    bool end() override {
        if (wire_ == nullptr) {
            return true;
        }
        return wire_->end();
    }

    bool setClock(uint32_t frequencyHz) override {
        return wire_ != nullptr && wire_->setClock(frequencyHz);
    }

    uint32_t getClock() const override {
        return wire_ != nullptr ? wire_->getClock() : 0U;
    }

    void beginTransmission(uint8_t address) override {
        if (wire_ != nullptr) {
            wire_->beginTransmission(address);
        }
    }

    uint8_t endTransmission(bool sendStop) override {
        return wire_ != nullptr ? wire_->endTransmission(sendStop) : 4U;
    }

    size_t requestFrom(uint8_t address, size_t size, bool sendStop) override {
        return wire_ != nullptr ? wire_->requestFrom(address, size, sendStop) : 0U;
    }

    size_t write(uint8_t data) override {
        return wire_ != nullptr ? wire_->write(data) : 0U;
    }

    size_t write(const uint8_t* data, size_t quantity) override {
        return wire_ != nullptr ? wire_->write(data, quantity) : 0U;
    }

    int available() override {
        return wire_ != nullptr ? wire_->available() : 0;
    }

    int read() override {
        return wire_ != nullptr ? wire_->read() : -1;
    }

    void flush() override {
        if (wire_ != nullptr) {
            wire_->flush();
        }
    }

private:
    std::unique_ptr<TwoWire> wire_{};
};

II2cBusDriver& defaultArduinoI2cBusDriver() {
    static ArduinoI2cBusDriver driver;
    return driver;
}

std::unique_ptr<II2cBusDriver> createArduinoI2cBusDriver() {
    return std::unique_ptr<II2cBusDriver>(new ArduinoI2cBusDriver());
}
#else
class NullI2cBusDriver final : public II2cBusDriver {
public:
    bool begin(uint8_t, uint8_t, uint32_t, bool) override {
        return false;
    }

    bool end() override {
        return true;
    }

    bool setClock(uint32_t) override {
        return false;
    }

    uint32_t getClock() const override {
        return 0U;
    }

    void beginTransmission(uint8_t) override {}

    uint8_t endTransmission(bool) override {
        return 4U;
    }

    size_t requestFrom(uint8_t, size_t, bool) override {
        return 0U;
    }

    size_t write(uint8_t) override {
        return 0U;
    }

    size_t write(const uint8_t*, size_t) override {
        return 0U;
    }

    int available() override {
        return 0;
    }

    int read() override {
        return -1;
    }

    void flush() override {}
};

II2cBusDriver& defaultArduinoI2cBusDriver() {
    static NullI2cBusDriver driver;
    return driver;
}

std::unique_ptr<II2cBusDriver> createArduinoI2cBusDriver() {
    return std::unique_ptr<II2cBusDriver>(new NullI2cBusDriver());
}
#endif

} // namespace ewfm
