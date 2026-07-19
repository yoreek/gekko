#include "devices/analog/composer/AnalogOutputComposerDeviceConfig.h"

#include "devices/core/ConfigCodec.h"

#include <type_traits>

namespace ewfm {

static_assert(std::is_trivially_copyable<AnalogOutputComposerDeviceConfigV1>::value, "AnalogOutputComposerDeviceConfigV1 must be POD");
static_assert(sizeof(AnalogOutputComposerDeviceConfigV1) == 34U, "AnalogOutputComposerDeviceConfigV1 layout changed");

bool AnalogOutputComposerDeviceConfigV1::parseJson(const JsonObjectConst& input, const char*& error) {
    return DeviceBaseConfigV1::parseJson(input, error);
}

DeviceValidationResult AnalogOutputComposerDeviceConfigV1::validate() const {
    return DeviceBaseConfigV1::validate();
}

void AnalogOutputComposerDeviceConfigV1::writeJson(JsonObject output) const {
    DeviceBaseConfigV1::writeJson(output);
}

bool decodeAnalogOutputComposerDeviceConfig(const uint8_t* blob, const size_t size, AnalogOutputComposerDeviceConfigV1& config) {
    return decodeValidatedFixedConfigBlob(AnalogOutputComposerDeviceConfigV1::kMagic, blob, size, config);
}

} // namespace ewfm
