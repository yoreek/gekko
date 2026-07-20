#include "devices/analog/input/ads1115/Ads1115Protocol.h"

#include <cstring>

namespace ewfm {

namespace {
constexpr uint16_t kSpsByRate[8] = {8, 16, 32, 64, 128, 250, 475, 860};
constexpr int32_t kFsrMilliVoltsByGain[6] = {6144, 4096, 2048, 1024, 512, 256};
} // namespace

bool ads1115GainFromByte(uint8_t value, Ads1115Gain& gain) {
    if (value > static_cast<uint8_t>(Ads1115Gain::Fsr0256)) {
        return false;
    }
    gain = static_cast<Ads1115Gain>(value);
    return true;
}

bool ads1115GainFromString(const char* value, Ads1115Gain& gain) {
    if (value == nullptr) {
        return false;
    }
    if (std::strcmp(value, "fsr6144") == 0) {
        gain = Ads1115Gain::Fsr6144;
        return true;
    }
    if (std::strcmp(value, "fsr4096") == 0) {
        gain = Ads1115Gain::Fsr4096;
        return true;
    }
    if (std::strcmp(value, "fsr2048") == 0) {
        gain = Ads1115Gain::Fsr2048;
        return true;
    }
    if (std::strcmp(value, "fsr1024") == 0) {
        gain = Ads1115Gain::Fsr1024;
        return true;
    }
    if (std::strcmp(value, "fsr0512") == 0) {
        gain = Ads1115Gain::Fsr0512;
        return true;
    }
    if (std::strcmp(value, "fsr0256") == 0) {
        gain = Ads1115Gain::Fsr0256;
        return true;
    }
    return false;
}

const char* ads1115GainName(Ads1115Gain gain) {
    switch (gain) {
    case Ads1115Gain::Fsr6144:
        return "fsr6144";
    case Ads1115Gain::Fsr4096:
        return "fsr4096";
    case Ads1115Gain::Fsr2048:
        return "fsr2048";
    case Ads1115Gain::Fsr1024:
        return "fsr1024";
    case Ads1115Gain::Fsr0512:
        return "fsr0512";
    case Ads1115Gain::Fsr0256:
        return "fsr0256";
    }
    return "fsr2048";
}

int32_t ads1115GainFullScaleMilliVolts(Ads1115Gain gain) {
    return kFsrMilliVoltsByGain[static_cast<uint8_t>(gain)];
}

bool ads1115DataRateFromByte(uint8_t value, Ads1115DataRate& dataRate) {
    if (value > static_cast<uint8_t>(Ads1115DataRate::Sps860)) {
        return false;
    }
    dataRate = static_cast<Ads1115DataRate>(value);
    return true;
}

bool ads1115DataRateFromString(const char* value, Ads1115DataRate& dataRate) {
    if (value == nullptr) {
        return false;
    }
    if (std::strcmp(value, "8") == 0) {
        dataRate = Ads1115DataRate::Sps8;
        return true;
    }
    if (std::strcmp(value, "16") == 0) {
        dataRate = Ads1115DataRate::Sps16;
        return true;
    }
    if (std::strcmp(value, "32") == 0) {
        dataRate = Ads1115DataRate::Sps32;
        return true;
    }
    if (std::strcmp(value, "64") == 0) {
        dataRate = Ads1115DataRate::Sps64;
        return true;
    }
    if (std::strcmp(value, "128") == 0) {
        dataRate = Ads1115DataRate::Sps128;
        return true;
    }
    if (std::strcmp(value, "250") == 0) {
        dataRate = Ads1115DataRate::Sps250;
        return true;
    }
    if (std::strcmp(value, "475") == 0) {
        dataRate = Ads1115DataRate::Sps475;
        return true;
    }
    if (std::strcmp(value, "860") == 0) {
        dataRate = Ads1115DataRate::Sps860;
        return true;
    }
    return false;
}

const char* ads1115DataRateName(Ads1115DataRate dataRate) {
    switch (dataRate) {
    case Ads1115DataRate::Sps8:
        return "8";
    case Ads1115DataRate::Sps16:
        return "16";
    case Ads1115DataRate::Sps32:
        return "32";
    case Ads1115DataRate::Sps64:
        return "64";
    case Ads1115DataRate::Sps128:
        return "128";
    case Ads1115DataRate::Sps250:
        return "250";
    case Ads1115DataRate::Sps475:
        return "475";
    case Ads1115DataRate::Sps860:
        return "860";
    }
    return "128";
}

uint16_t ads1115ConfigRegisterValue(uint8_t channel, Ads1115Gain gain, Ads1115DataRate dataRate) {
    uint16_t config = 0;
    config |= 0x1U << 15;                                                    // OS: start a single conversion
    config |= static_cast<uint16_t>((0x4U + (channel & 0x3U)) & 0x7U) << 12; // MUX: single-ended AINx vs GND
    config |= static_cast<uint16_t>(gain) << 9;                              // PGA
    config |= 0x1U << 8;                                                     // MODE: single-shot
    config |= static_cast<uint16_t>(dataRate) << 5;                          // DR
    config |= 0x3U;                                                          // COMP_QUE: disable comparator
    return config;
}

uint32_t ads1115ConversionTimeMs(Ads1115DataRate dataRate) {
    const uint16_t sps = kSpsByRate[static_cast<uint8_t>(dataRate)];
    const uint32_t baseMs = (1000U + sps - 1U) / sps; // ceil(1000/sps)
    return baseMs + 1U;                               // safety margin
}

int32_t ads1115RawToMilliVolts(int16_t raw, Ads1115Gain gain) {
    return (static_cast<int32_t>(raw) * ads1115GainFullScaleMilliVolts(gain)) / 32768;
}

} // namespace ewfm
