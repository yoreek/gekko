#pragma once

#include "devices/pulse/RmtPulseCapture.h"

#include <cstddef>
#include <cstdint>

namespace ewfm {

constexpr uint8_t kDht11FrameBytes = 5U;
constexpr size_t kDht11DataPulseCount = 40U * 2U;

bool dht11DecodePulsePairs(const PulseCaptureSample* samples, size_t sampleCount, int32_t& milliCelsius, int32_t& milliPercent,
                           const char*& errorStatus);

// Returns the first data pulse after the DHT response preamble, or sampleCount when absent.
size_t dht11DataPulseStart(const PulseCaptureSample* samples, size_t sampleCount);

// Finds and decodes a complete DHT frame within a capture that may also contain the controller
// start pulse and the sensor response preamble.
bool dht11DecodeFramedPulsePairs(const PulseCaptureSample* samples, size_t sampleCount, int32_t& milliCelsius, int32_t& milliPercent,
                                 const char*& errorStatus);

} // namespace ewfm
