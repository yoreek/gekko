#pragma once

#include <cstdint>

namespace ewfm {

// AHT10 command set and integer-only conversions. The driver uses init + trigger-measure
// commands, then reads the 6-byte status/measurement frame on a later tick so the bus never
// blocks across the conversion delay.
constexpr uint8_t kAht10CmdInit = 0xE1;
constexpr uint8_t kAht10CmdMeasure = 0xAC;
constexpr uint8_t kAht10CmdInitArg1 = 0x08;
constexpr uint8_t kAht10CmdInitArg2 = 0x00;
constexpr uint8_t kAht10CmdMeasureArg1 = 0x33;
constexpr uint8_t kAht10CmdMeasureArg2 = 0x00;

constexpr uint32_t kAht10InitMs = 20;
constexpr uint32_t kAht10MeasureMs = 80;
constexpr uint8_t kAht10MeasureResponseBytes = 6;
constexpr uint8_t kAht10StatusBusy = 0x80;
constexpr uint8_t kAht10StatusCalibrated = 0x08;

bool aht10MeasurementReady(uint8_t status);
bool aht10MeasurementCalibrated(uint8_t status);
bool aht10DecodeMeasurement(const uint8_t* frame, uint32_t& humidityRaw, uint32_t& temperatureRaw, const char*& errorStatus);
int32_t aht10RawToMilliCelsius(uint32_t raw);
int32_t aht10RawToMilliPercent(uint32_t raw);

} // namespace ewfm
