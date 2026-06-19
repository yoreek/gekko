#include "devices/sensors/ds18b20/Ds18b20OneWireProtocol.h"

namespace ewfm {

bool ds18b20AddressIsValid(const OneWireRomAddress& address) {
    return address.bytes[0] == kDs18b20FamilyCode && oneWireRomCrcValid(address);
}

bool ds18b20ResolutionIsValid(uint8_t resolution) {
    return resolution >= 9U && resolution <= 12U;
}

uint8_t ds18b20ResolutionConfigByte(uint8_t resolution) {
    switch (resolution) {
    case 9:
        return 0x1FU;
    case 10:
        return 0x3FU;
    case 11:
        return 0x5FU;
    case 12:
    default:
        return 0x7FU;
    }
}

bool ds18b20ResolutionFromConfigByte(uint8_t configByte, uint8_t& resolution) {
    switch (configByte & 0x60U) {
    case 0x00U:
        resolution = 9;
        return true;
    case 0x20U:
        resolution = 10;
        return true;
    case 0x40U:
        resolution = 11;
        return true;
    case 0x60U:
        resolution = 12;
        return true;
    default:
        return false;
    }
}

uint16_t ds18b20ConversionTimeMs(uint8_t resolution) {
    switch (resolution) {
    case 9:
        return 94U;
    case 10:
        return 188U;
    case 11:
        return 375U;
    case 12:
    default:
        return 750U;
    }
}

bool ds18b20ScratchpadCrcValid(const uint8_t (&scratchpad)[kDs18b20ScratchpadSize]) {
    return oneWireCrc8(scratchpad, kDs18b20ScratchpadSize - 1U) == scratchpad[kDs18b20ScratchpadSize - 1U];
}

bool ds18b20ParseScratchpadTemperature(const uint8_t (&scratchpad)[kDs18b20ScratchpadSize], int32_t& milliCelsius) {
    if (!ds18b20ScratchpadCrcValid(scratchpad)) {
        return false;
    }

    const int16_t raw = static_cast<int16_t>((static_cast<uint16_t>(scratchpad[1]) << 8U) | scratchpad[0]);
    const int32_t parsedMilliCelsius = (static_cast<int32_t>(raw) * 625) / 10;
    if (parsedMilliCelsius < -55000 || parsedMilliCelsius > 125000) {
        return false;
    }

    milliCelsius = parsedMilliCelsius;
    return true;
}

} // namespace ewfm
