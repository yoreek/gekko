#pragma once

#include "devices/core/DeviceTypes.h"

#include <cstddef>
#include <cstdint>

namespace ewfm {

struct OneWireRomAddress {
    uint8_t bytes[8]{};
};

class IOneWireBusDriver {
public:
    IOneWireBusDriver() = default;
    IOneWireBusDriver(const IOneWireBusDriver&) = delete;
    IOneWireBusDriver& operator=(const IOneWireBusDriver&) = delete;
    IOneWireBusDriver(IOneWireBusDriver&&) = delete;
    IOneWireBusDriver& operator=(IOneWireBusDriver&&) = delete;
    virtual ~IOneWireBusDriver() = default;

    virtual bool begin(uint8_t pin, bool internalPullup) = 0;
    virtual void depower() = 0;
    virtual void resetSearch() = 0;
    virtual bool search(OneWireRomAddress& address) = 0;
    virtual uint8_t crc8(const uint8_t* data, size_t len) const = 0;
};

bool formatOneWireRomAddress(const OneWireRomAddress& address, char (&out)[17]);
bool parseOneWireRomAddress(const char* input, OneWireRomAddress& address);
bool oneWireRomCrcValid(const IOneWireBusDriver& driver, const OneWireRomAddress& address);

} // namespace ewfm
