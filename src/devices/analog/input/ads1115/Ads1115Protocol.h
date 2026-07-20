#pragma once

#include <cstdint>

namespace ewfm {

// Register pointers (ADS1115 datasheet Table 7).
constexpr uint8_t kAds1115RegConversion = 0x00;
constexpr uint8_t kAds1115RegConfig = 0x01;

constexpr uint8_t kAds1115ChannelCount = 4; // AIN0..AIN3, single-ended only in this driver

// Programmable gain amplifier full-scale range, in millivolts. Larger FSR = lower resolution but
// tolerates a wider input swing; smaller FSR = finer resolution over a narrower input.
enum class Ads1115Gain : uint8_t {
    Fsr6144 = 0,
    Fsr4096 = 1,
    Fsr2048 = 2,
    Fsr1024 = 3,
    Fsr0512 = 4,
    Fsr0256 = 5,
};

enum class Ads1115DataRate : uint8_t {
    Sps8 = 0,
    Sps16 = 1,
    Sps32 = 2,
    Sps64 = 3,
    Sps128 = 4,
    Sps250 = 5,
    Sps475 = 6,
    Sps860 = 7,
};

bool ads1115GainFromByte(uint8_t value, Ads1115Gain& gain);
bool ads1115GainFromString(const char* value, Ads1115Gain& gain);
const char* ads1115GainName(Ads1115Gain gain);
int32_t ads1115GainFullScaleMilliVolts(Ads1115Gain gain);

bool ads1115DataRateFromByte(uint8_t value, Ads1115DataRate& dataRate);
bool ads1115DataRateFromString(const char* value, Ads1115DataRate& dataRate);
const char* ads1115DataRateName(Ads1115DataRate dataRate);

// Builds the 16-bit config register value that starts a single-shot conversion on `channel`
// (0..3, single-ended against GND) with the given gain/data rate and the comparator disabled.
uint16_t ads1115ConfigRegisterValue(uint8_t channel, Ads1115Gain gain, Ads1115DataRate dataRate);

// Conservative conversion time for `dataRate`, rounded up from 1000/sps with headroom so a
// dependent's poll after this many milliseconds reliably observes OS=1 (conversion complete).
uint32_t ads1115ConversionTimeMs(Ads1115DataRate dataRate);

// Converts a signed 16-bit conversion register reading to millivolts for the given gain.
int32_t ads1115RawToMilliVolts(int16_t raw, Ads1115Gain gain);

} // namespace ewfm
