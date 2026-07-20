#include "devices/analog/adc/AdcAttenuationCodec.h"

#include <cstring>

namespace ewfm {

bool attenuationFromByte(uint8_t value, AdcAttenuation& attenuation) {
    if (value > static_cast<uint8_t>(AdcAttenuation::Db11)) {
        return false;
    }
    attenuation = static_cast<AdcAttenuation>(value);
    return true;
}

bool attenuationFromString(const char* value, AdcAttenuation& attenuation) {
    if (value == nullptr) {
        return false;
    }
    if (std::strcmp(value, "0db") == 0) {
        attenuation = AdcAttenuation::Db0;
        return true;
    }
    if (std::strcmp(value, "2_5db") == 0) {
        attenuation = AdcAttenuation::Db2_5;
        return true;
    }
    if (std::strcmp(value, "6db") == 0) {
        attenuation = AdcAttenuation::Db6;
        return true;
    }
    if (std::strcmp(value, "11db") == 0) {
        attenuation = AdcAttenuation::Db11;
        return true;
    }
    return false;
}

const char* attenuationName(AdcAttenuation attenuation) {
    switch (attenuation) {
    case AdcAttenuation::Db0:
        return "0db";
    case AdcAttenuation::Db2_5:
        return "2_5db";
    case AdcAttenuation::Db6:
        return "6db";
    case AdcAttenuation::Db11:
        return "11db";
    }
    return "11db";
}

} // namespace ewfm
