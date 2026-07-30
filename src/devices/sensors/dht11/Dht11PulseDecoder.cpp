#include "devices/sensors/dht11/Dht11PulseDecoder.h"

#include "devices/sensors/dht11/Dht11Protocol.h"

namespace ewfm {

namespace {

bool decodeDht11PulseBits(const PulseCaptureSample* samples, uint8_t bitCount, uint8_t* frame, const char*& errorStatus) {
    for (uint8_t bitIndex = 0U; bitIndex < bitCount; ++bitIndex) {
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
    return true;
}

} // namespace

size_t dht11DataPulseStart(const PulseCaptureSample* samples, size_t sampleCount) {
    constexpr uint16_t kPreambleMinMicros = 60U;
    constexpr uint16_t kPreambleMaxMicros = 100U;
    if (samples == nullptr) {
        return sampleCount;
    }
    // The RMT receiver may omit the final high pulse, leaving 79 data segments.
    constexpr size_t kMinimumDataPulseCount = kDht11DataPulseCount - 1U;
    for (size_t index = 0U; index + 2U + kMinimumDataPulseCount <= sampleCount; ++index) {
        const PulseCaptureSample& low = samples[index];
        const PulseCaptureSample& high = samples[index + 1U];
        if (!low.level && high.level && low.durationMicros >= kPreambleMinMicros && low.durationMicros <= kPreambleMaxMicros &&
            high.durationMicros >= kPreambleMinMicros && high.durationMicros <= kPreambleMaxMicros) {
            return index + 2U;
        }
    }
    return sampleCount;
}

bool dht11DecodePulsePairs(const PulseCaptureSample* samples, size_t sampleCount, int32_t& milliCelsius, int32_t& milliPercent,
                           const char*& errorStatus) {
    if (sampleCount < kDht11DataPulseCount) {
        errorStatus = "timeout";
        return false;
    }
    uint8_t frame[kDht11FrameBytes]{};
    if (!decodeDht11PulseBits(samples, 40U, frame, errorStatus)) {
        return false;
    }
    return dht11DecodeFrame(frame, milliCelsius, milliPercent, errorStatus);
}

bool dht11DecodeFramedPulsePairs(const PulseCaptureSample* samples, size_t sampleCount, int32_t& milliCelsius, int32_t& milliPercent,
                                 const char*& errorStatus) {
    if (samples == nullptr || sampleCount < kDht11DataPulseCount) {
        errorStatus = "timeout";
        return false;
    }
    const size_t start = dht11DataPulseStart(samples, sampleCount);
    if (start == sampleCount) {
        errorStatus = "protocol_error";
        return false;
    }
    if (start + kDht11DataPulseCount <= sampleCount) {
        return dht11DecodePulsePairs(samples + start, sampleCount - start, milliCelsius, milliPercent, errorStatus);
    }

    // Arduino-ESP32's RMT ring buffer can omit the final high pulse: it ends in the idle
    // threshold rather than an edge. The preceding low pulse and the first 39 bits are still
    // present. The missing bit is the checksum LSB, so reconstruct it from the four data bytes.
    if (start + kDht11DataPulseCount - 1U > sampleCount || samples[start + kDht11DataPulseCount - 2U].level) {
        errorStatus = "timeout";
        return false;
    }
    uint8_t frame[kDht11FrameBytes]{};
    if (!decodeDht11PulseBits(samples + start, 39U, frame, errorStatus)) {
        return false;
    }
    const uint8_t expectedChecksum = static_cast<uint8_t>(frame[0] + frame[1] + frame[2] + frame[3]);
    if ((frame[4] & 0xFEU) != (expectedChecksum & 0xFEU)) {
        errorStatus = "checksum_error";
        return false;
    }
    frame[4] = expectedChecksum;
    return dht11DecodeFrame(frame, milliCelsius, milliPercent, errorStatus);
}

} // namespace ewfm
