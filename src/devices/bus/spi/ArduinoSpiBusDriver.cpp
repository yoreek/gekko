#include "devices/bus/spi/ISpiBusDriver.h"

#if defined(ARDUINO) && !defined(UNIT_TEST)
#include "devices/bus/spi/SpiBusConfig.h"

#include <Arduino.h>
#include <SPI.h>
#endif

#include <memory>

namespace ewfm {

#if defined(ARDUINO) && !defined(UNIT_TEST)
namespace {
uint8_t spiHostToArduinoHost(uint8_t host) {
    switch (host) {
    case kSpiBusHostHspi:
        return static_cast<uint8_t>(HSPI);
    case kSpiBusHostVspi:
    default:
        return static_cast<uint8_t>(VSPI);
    }
}
} // namespace

class ArduinoSpiBusDriver final : public ISpiBusDriver {
public:
    bool begin(uint8_t host, uint8_t sckPin, uint8_t mosiPin, int16_t misoPin) override {
        bus_.reset(new SPIClass(spiHostToArduinoHost(host)));
        if (bus_ == nullptr) {
            return false;
        }
        const int8_t miso = misoPin >= 0 ? static_cast<int8_t>(misoPin) : static_cast<int8_t>(-1);
        bus_->begin(static_cast<int8_t>(sckPin), miso, static_cast<int8_t>(mosiPin), static_cast<int8_t>(-1));
        begun_ = true;
        return true;
    }

    bool end() override {
        if (!begun_ || bus_ == nullptr) {
            transactionActive_ = false;
            return true;
        }
        if (transactionActive_) {
            bus_->endTransaction();
            transactionActive_ = false;
        }
        bus_->end();
        begun_ = false;
        return true;
    }

    bool beginTransaction(uint32_t clockHz, uint8_t dataMode, uint8_t bitOrder) override {
        if (bus_ == nullptr || transactionActive_) {
            return false;
        }
        settings_.reset(new SPISettings(clockHz, bitOrder, dataMode));
        if (settings_ == nullptr) {
            return false;
        }
        bus_->beginTransaction(*settings_);
        transactionActive_ = true;
        return true;
    }

    void endTransaction() override {
        if (bus_ != nullptr && transactionActive_) {
            bus_->endTransaction();
        }
        transactionActive_ = false;
    }

    uint8_t transfer(uint8_t data) override {
        return bus_ != nullptr ? bus_->transfer(data) : 0U;
    }

    size_t transferBytes(const uint8_t* txData, uint8_t* rxData, size_t length) override {
        if (bus_ == nullptr) {
            return 0U;
        }
        for (size_t index = 0; index < length; ++index) {
            const uint8_t tx = txData != nullptr ? txData[index] : 0xFFU;
            const uint8_t rx = bus_->transfer(tx);
            if (rxData != nullptr) {
                rxData[index] = rx;
            }
        }
        return length;
    }

    SPIClass* nativeSpi() const override {
        return bus_.get();
    }

private:
    std::unique_ptr<SPIClass> bus_{};
    std::unique_ptr<SPISettings> settings_{};
    bool begun_{false};
    bool transactionActive_{false};
};

ISpiBusDriver& defaultArduinoSpiBusDriver() {
    static ArduinoSpiBusDriver driver;
    return driver;
}

std::unique_ptr<ISpiBusDriver> createArduinoSpiBusDriver() {
    return std::unique_ptr<ISpiBusDriver>(new ArduinoSpiBusDriver());
}
#else
class NullSpiBusDriver final : public ISpiBusDriver {
public:
    bool begin(uint8_t, uint8_t, uint8_t, int16_t) override {
        return false;
    }

    bool end() override {
        return true;
    }

    bool beginTransaction(uint32_t, uint8_t, uint8_t) override {
        return false;
    }

    void endTransaction() override {}

    uint8_t transfer(uint8_t data) override {
        return data;
    }

    size_t transferBytes(const uint8_t*, uint8_t*, size_t length) override {
        return length;
    }
};

ISpiBusDriver& defaultArduinoSpiBusDriver() {
    static NullSpiBusDriver driver;
    return driver;
}

std::unique_ptr<ISpiBusDriver> createArduinoSpiBusDriver() {
    return std::unique_ptr<ISpiBusDriver>(new NullSpiBusDriver());
}
#endif

} // namespace ewfm
