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

    uint8_t crc8(const uint8_t* data, size_t len) const override {
        if (bus_ == nullptr) {
            return 0;
        }
        return bus_->crc8(data, len);
    }

private:
    std::unique_ptr<OneWire> bus_{};
};

IOneWireBusDriver& defaultArduinoOneWireBusDriver() {
    static ArduinoOneWireBusDriver driver;
    return driver;
}
#else
class NullOneWireBusDriver final : public IOneWireBusDriver {
public:
    bool begin(uint8_t, bool) override {
        return false;
    }

    void depower() override {}

    void resetSearch() override {}

    bool search(OneWireRomAddress&) override {
        return false;
    }

    uint8_t crc8(const uint8_t*, size_t) const override {
        return 0;
    }
};

IOneWireBusDriver& defaultArduinoOneWireBusDriver() {
    static NullOneWireBusDriver driver;
    return driver;
}
#endif

} // namespace ewfm
