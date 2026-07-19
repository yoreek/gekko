#include "devices/bus/onewire/IOneWireBusDriver.h"

#if defined(ARDUINO) && !defined(UNIT_TEST)
#include <Arduino.h>
#include <OneWire.h>
#endif

#include <memory>

namespace ewfm {

#if defined(ARDUINO) && !defined(UNIT_TEST)
class ArduinoOneWireBusDriver final : public IOneWireBusDriver {
public:
    bool begin(uint8_t pin, bool internalPullup) override {
        bus_.reset(new OneWire(pin));
        if (bus_ == nullptr) {
            return false;
        }
        pinMode(pin, internalPullup ? INPUT_PULLUP : INPUT);
        return true;
    }

    void depower() override {
        if (bus_ != nullptr) {
            bus_->depower();
        }
    }

    bool reset() override {
        return bus_ != nullptr && bus_->reset() != 0U;
    }

    void resetSearch() override {
        if (bus_ != nullptr) {
            bus_->reset_search();
        }
    }

    bool search(OneWireRomAddress& address) override {
        if (bus_ == nullptr) {
            return false;
        }
        return bus_->search(address.bytes);
    }

    void select(const OneWireRomAddress& address) override {
        if (bus_ != nullptr) {
            bus_->select(address.bytes);
        }
    }

    void skip() override {
        if (bus_ != nullptr) {
            bus_->skip();
        }
    }

    void write(uint8_t value, bool power = false) override {
        if (bus_ != nullptr) {
            bus_->write(value, power ? 1U : 0U);
        }
    }

    uint8_t read() override {
        return bus_ != nullptr ? bus_->read() : 0U;
    }

    uint8_t readBit() override {
        return bus_ != nullptr ? bus_->read_bit() : 0U;
    }

    uint8_t crc8(const uint8_t* data, size_t len) const override {
        return OneWire::crc8(data, len);
    }

private:
    std::unique_ptr<OneWire> bus_{};
};

IOneWireBusDriver& defaultArduinoOneWireBusDriver() {
    static ArduinoOneWireBusDriver driver;
    return driver;
}

std::unique_ptr<IOneWireBusDriver> createArduinoOneWireBusDriver() {
    return std::unique_ptr<IOneWireBusDriver>(new ArduinoOneWireBusDriver());
}
#else
class NullOneWireBusDriver final : public IOneWireBusDriver {
public:
    bool begin(uint8_t, bool) override {
        return false;
    }

    void depower() override {}

    bool reset() override {
        return false;
    }

    void resetSearch() override {}

    bool search(OneWireRomAddress&) override {
        return false;
    }

    void select(const OneWireRomAddress&) override {}

    void skip() override {}

    void write(uint8_t, bool = false) override {}

    uint8_t read() override {
        return 0;
    }

    uint8_t readBit() override {
        return 0;
    }

    uint8_t crc8(const uint8_t* data, size_t len) const override {
        return oneWireCrc8(data, len);
    }
};

IOneWireBusDriver& defaultArduinoOneWireBusDriver() {
    static NullOneWireBusDriver driver;
    return driver;
}

std::unique_ptr<IOneWireBusDriver> createArduinoOneWireBusDriver() {
    return std::unique_ptr<IOneWireBusDriver>(new NullOneWireBusDriver());
}
#endif

} // namespace ewfm
