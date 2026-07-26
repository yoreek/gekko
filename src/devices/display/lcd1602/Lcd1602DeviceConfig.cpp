#include "devices/display/lcd1602/Lcd1602DeviceConfig.h"

#include "devices/core/ConfigCodec.h"

#include <cstring>
#include <type_traits>

namespace ewfm {

static_assert(std::is_trivially_copyable<Lcd1602DeviceConfigV1>::value, "Lcd1602DeviceConfigV1 must be POD");
static_assert(sizeof(Lcd1602DeviceConfigV1::kMagic) - 1U + sizeof(Lcd1602DeviceConfigV1) <= kMaxDeviceConfigBytes,
              "Lcd1602DeviceConfigV1 exceeds device config bound");

bool decodeLcd1602DeviceConfig(const uint8_t* blob, size_t size, Lcd1602DeviceConfigV1& config) {
    return decodeValidatedFixedConfigBlob(Lcd1602DeviceConfigV1::kMagic, blob, size, config);
}

bool Lcd1602DeviceConfigV1::parseJson(const JsonObjectConst& input, const char*& error) {
    if (!DeviceBaseConfigV1::parseJson(input, error)) {
        return false;
    }
    if (!channels.parseJson(input, error)) {
        return false;
    }

    char previousLine1[kLcd1602LineLength + 1U]{};
    std::memcpy(previousLine1, line1, sizeof(previousLine1));
    const char* newLine1 = input["line1"] | static_cast<const char*>(previousLine1);
    if (!copyBoundedText(line1, newLine1)) {
        error = "lcd1602 line1 exceeds 16 characters";
        return false;
    }

    char previousLine2[kLcd1602LineLength + 1U]{};
    std::memcpy(previousLine2, line2, sizeof(previousLine2));
    const char* newLine2 = input["line2"] | static_cast<const char*>(previousLine2);
    if (!copyBoundedText(line2, newLine2)) {
        error = "lcd1602 line2 exceeds 16 characters";
        return false;
    }

    return true;
}

DeviceValidationResult Lcd1602DeviceConfigV1::validate() const {
    const DeviceValidationResult baseResult = DeviceBaseConfigV1::validate();
    if (!baseResult.ok()) {
        return baseResult;
    }
    return channels.validate();
}

void Lcd1602DeviceConfigV1::writeJson(JsonObject output) const {
    DeviceBaseConfigV1::writeJson(output);
    channels.writeJson(output);
    output["line1"] = JsonString(line1, JsonString::Copied);
    output["line2"] = JsonString(line2, JsonString::Copied);
}

} // namespace ewfm
