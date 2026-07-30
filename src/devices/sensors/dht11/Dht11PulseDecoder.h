#pragma once

#include "devices/pulse/RmtPulseCapture.h"

#include <cstddef>
#include <cstdint>

namespace ewfm {

constexpr uint8_t kDht11FrameBytes = 5U;
constexpr size_t kDht11DataPulseCount = 40U * 2U;

bool dht11DecodePulsePairs(const PulseCaptureSample* samples, size_t sampleCount, int32_t& milliCelsius, int32_t& milliPercent,
                           const char*& errorStatus);

} // namespace ewfm
