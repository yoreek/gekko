#pragma once

#include "devices/sensors/dht11/Dht11LineDriver.h"

#include <cstddef>
#include <cstdint>

namespace ewfm {

constexpr uint32_t kDht11DefaultPowerOnDelayMs = 1000U;
constexpr uint8_t kDht11FrameBytes = 5;
constexpr uint32_t kDht11StartPulseMicros = 18000U;
constexpr uint32_t kDht11ReleaseWaitMicros = 30U;
constexpr uint32_t kDht11ResponseTimeoutMicros = 120U;

bool dht11ChecksumValid(const uint8_t* frame);
bool dht11DecodeFrame(const uint8_t* frame, int32_t& milliCelsius, int32_t& milliPercent, const char*& errorStatus);
bool dht11CaptureMeasurement(IDht11LineDriver& driver, uint8_t pin, bool internalPullup, int32_t& milliCelsius, int32_t& milliPercent,
                             const char*& errorStatus);
int32_t dht11RawToMilliCelsius(uint8_t temperatureInteger, uint8_t temperatureDecimal);
int32_t dht11RawToMilliPercent(uint8_t humidityInteger, uint8_t humidityDecimal);

} // namespace ewfm
