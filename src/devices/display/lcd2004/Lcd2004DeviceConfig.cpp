#include "devices/display/lcd2004/Lcd2004DeviceConfig.h"

#include "devices/core/ConfigCodec.h"

#include <cstring>
#include <type_traits>

namespace ewfm {

static_assert(std::is_trivially_copyable<Lcd2004DeviceConfigV1>::value, "Lcd2004DeviceConfigV1 must be POD");
static_assert(sizeof(Lcd2004DeviceConfigV1::kMagic) - 1U + sizeof(Lcd2004DeviceConfigV1) <= kMaxDeviceConfigBytes,
              "Lcd2004DeviceConfigV1 exceeds device config bound");

bool decodeLcd2004DeviceConfig(const uint8_t* blob, size_t size, Lcd2004DeviceConfigV1& config) {
    return decodeValidatedFixedConfigBlob(Lcd2004DeviceConfigV1::kMagic, blob, size, config);
}

bool Lcd2004DeviceConfigV1::parseJson(const JsonObjectConst& input, const char*& error) {
    if (!DeviceBaseConfigV1::parseJson(input, error)) {
        return false;
    }
    if (!channels.parseJson(input, error)) {
        return false;
    }

    char previousLine1[kLcd2004LineLength + 1U]{};
    std::memcpy(previousLine1, line1, sizeof(previousLine1));
    const char* newLine1 = input["line1"] | static_cast<const char*>(previousLine1);
    if (!copyBoundedText(line1, newLine1)) {
        error = "lcd2004 line1 exceeds 20 characters";
        return false;
    }

    char previousLine2[kLcd2004LineLength + 1U]{};
    std::memcpy(previousLine2, line2, sizeof(previousLine2));
    const char* newLine2 = input["line2"] | static_cast<const char*>(previousLine2);
    if (!copyBoundedText(line2, newLine2)) {
        error = "lcd2004 line2 exceeds 20 characters";
        return false;
    }

    char previousLine3[kLcd2004LineLength + 1U]{};
    std::memcpy(previousLine3, line3, sizeof(previousLine3));
    const char* newLine3 = input["line3"] | static_cast<const char*>(previousLine3);
    if (!copyBoundedText(line3, newLine3)) {
        error = "lcd2004 line3 exceeds 20 characters";
        return false;
    }

    char previousLine4[kLcd2004LineLength + 1U]{};
    std::memcpy(previousLine4, line4, sizeof(previousLine4));
    const char* newLine4 = input["line4"] | static_cast<const char*>(previousLine4);
    if (!copyBoundedText(line4, newLine4)) {
        error = "lcd2004 line4 exceeds 20 characters";
        return false;
    }

    return true;
}

DeviceValidationResult Lcd2004DeviceConfigV1::validate() const {
    const DeviceValidationResult baseResult = DeviceBaseConfigV1::validate();
    if (!baseResult.ok()) {
        return baseResult;
    }
    return channels.validate();
}

void Lcd2004DeviceConfigV1::writeJson(JsonObject output) const {
    DeviceBaseConfigV1::writeJson(output);
    channels.writeJson(output);
    output["line1"] = JsonString(line1, JsonString::Copied);
    output["line2"] = JsonString(line2, JsonString::Copied);
    output["line3"] = JsonString(line3, JsonString::Copied);
    output["line4"] = JsonString(line4, JsonString::Copied);
}

} // namespace ewfm
