#pragma once

#include <cstdint>

namespace ewfm {

// HTU21(D)F command set and integer-only measurement conversions (datasheet formulas
// T = raw * 175.72 / 65536 - 46.85 degC, RH = raw * 125 / 65536 - 6 %). No-hold commands are
// used so the I2C bus is never clock-stretched across ticks; the caller waits out the
// conversion time itself before reading the 3-byte response (MSB, LSB, CRC).
constexpr uint8_t kHtu21CmdSoftReset = 0xFE;
constexpr uint8_t kHtu21CmdMeasureTemperatureNoHold = 0xF3;
constexpr uint8_t kHtu21CmdMeasureHumidityNoHold = 0xF5;

constexpr uint32_t kHtu21SoftResetMs = 20;          // datasheet: 15 ms max
constexpr uint32_t kHtu21TemperatureMeasureMs = 60; // datasheet: 50 ms max at 14-bit
constexpr uint32_t kHtu21HumidityMeasureMs = 25;    // datasheet: 16 ms max at 12-bit
constexpr uint8_t kHtu21MeasureResponseBytes = 3;

bool htu21CrcValid(uint16_t rawValue, uint8_t crc);
// Both take the raw 16-bit response with the two status bits still set; they mask them off.
int32_t htu21RawToMilliCelsius(uint16_t raw);
int32_t htu21RawToMilliPercent(uint16_t raw);

} // namespace ewfm
