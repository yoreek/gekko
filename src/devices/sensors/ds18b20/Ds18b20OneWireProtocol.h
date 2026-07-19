#pragma once

#include "devices/bus/onewire/OneWireRomAddress.h"

#include <cstdint>

namespace ewfm {

constexpr uint8_t kDs18b20FamilyCode = 0x28;
constexpr uint8_t kDs18b20CommandConvertT = 0x44;
constexpr uint8_t kDs18b20CommandReadScratchpad = 0xBE;
constexpr uint8_t kDs18b20CommandWriteScratchpad = 0x4E;
constexpr size_t kDs18b20ScratchpadSize = 9;

bool ds18b20AddressIsValid(const OneWireRomAddress& address);
bool ds18b20ResolutionIsValid(uint8_t resolution);
uint8_t ds18b20ResolutionConfigByte(uint8_t resolution);
bool ds18b20ResolutionFromConfigByte(uint8_t configByte, uint8_t& resolution);
uint16_t ds18b20ConversionTimeMs(uint8_t resolution);
bool ds18b20ScratchpadCrcValid(const uint8_t (&scratchpad)[kDs18b20ScratchpadSize]);
bool ds18b20ParseScratchpadTemperature(const uint8_t (&scratchpad)[kDs18b20ScratchpadSize], int32_t& milliCelsius);

} // namespace ewfm
