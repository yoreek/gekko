#include "devices/rtc/common/RtcSyncConfig.h"

namespace ewfm {

bool parseRtcSyncFieldJson(const JsonObjectConst& input, uint8_t& useForSystemTimeSync, const char*& error) {
    (void)error; // kept for signature symmetry with parseI2cAddressJson; this field always coerces.
    useForSystemTimeSync = (input["useForSystemTimeSync"] | (useForSystemTimeSync != 0U)) ? 1U : 0U;
    return true;
}

DeviceValidationResult validateRtcSyncField(uint8_t useForSystemTimeSync) {
    if (useForSystemTimeSync > 1U) {
        return {DeviceError::InvalidConfig, "useForSystemTimeSync is invalid"};
    }
    return {};
}

void writeRtcSyncFieldJson(uint8_t useForSystemTimeSync, JsonObject output) {
    output["useForSystemTimeSync"] = useForSystemTimeSync != 0U;
}

} // namespace ewfm
