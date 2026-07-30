#include "devices/sensors/dht11/Dht11PulseDecoder.h"

#include "devices/sensors/dht11/Dht11Protocol.h"

namespace ewfm {

bool dht11DecodePulsePairs(const PulseCaptureSample* samples, size_t sampleCount, int32_t& milliCelsius, int32_t& milliPercent,
                           const char*& errorStatus) {
    if (sampleCount < kDht11DataPulseCount) {
        errorStatus = "timeout";
        return false;
    }
    uint8_t frame[kDht11FrameBytes]{};
    for (uint8_t bitIndex = 0U; bitIndex < 40U; ++bitIndex) {
        const PulseCaptureSample& low = samples[bitIndex * 2U];
        const PulseCaptureSample& high = samples[bitIndex * 2U + 1U];
        if (low.level || !high.level) {
            errorStatus = "protocol_error";
            return false;
        }
        if (high.durationMicros > low.durationMicros) {
            frame[bitIndex / 8U] |= static_cast<uint8_t>(1U << (7U - (bitIndex % 8U)));
        }
    }
    return dht11DecodeFrame(frame, milliCelsius, milliPercent, errorStatus);
}

} // namespace ewfm
