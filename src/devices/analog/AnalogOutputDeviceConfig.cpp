#include "devices/analog/AnalogOutputDeviceConfig.h"

#include <type_traits>

namespace ewfm {

static_assert(std::is_trivially_copyable<AnalogOutputDeviceConfigV1>::value, "AnalogOutputDeviceConfigV1 must be POD");
static_assert(sizeof(AnalogOutputDeviceConfigV1) == 40U, "AnalogOutputDeviceConfigV1 layout changed");

bool OutputDeviceValueCodec<uint16_t>::parseJson(const JsonVariantConst& input, uint16_t& state, const char*& error) {
    if (!input.is<unsigned long>() && !input.is<long>() && !input.is<int>()) {
        error = "output state value must be numeric";
        return false;
    }
    const long percent = input.as<long>();
    if (percent < 0L || percent > 100L) {
        error = "output state value is out of bounds";
        return false;
    }
    state = percentToAnalogOutputState(static_cast<uint16_t>(percent));
    return true;
}

bool OutputDeviceValueCodec<uint16_t>::valid(const uint16_t state) {
    return state <= kAnalogOutputLevelMax;
}

void OutputDeviceValueCodec<uint16_t>::writeJson(JsonObject output, const char* key, const uint16_t state) {
    output[key] = analogOutputStateToPercent(state);
}

} // namespace ewfm
