#include "devices/dummy/DummyDeviceConfigCodec.h"

#include "devices/dummy/DummyDevice.h"

#include <type_traits>

namespace ewfm {

namespace {
template <typename T> void appendLE(std::string& out, T value) {
    using Unsigned = typename std::make_unsigned<T>::type;
    const Unsigned v = static_cast<Unsigned>(value);
    for (size_t index = 0; index < sizeof(T); ++index) {
        out.push_back(static_cast<char>((v >> (index * 8)) & 0xFFU));
    }
}

template <typename T> bool readLE(const std::string& blob, size_t& pos, T& value) {
    using Unsigned = typename std::make_unsigned<T>::type;
    if (pos + sizeof(T) > blob.size()) {
        return false;
    }
    Unsigned v{0};
    for (size_t index = 0; index < sizeof(T); ++index) {
        v |= static_cast<Unsigned>(static_cast<unsigned char>(blob[pos + index])) << (index * 8);
    }
    value = static_cast<T>(v);
    pos += sizeof(T);
    return true;
}

} // namespace

std::string encodeDummyDeviceConfig(const DummyDeviceConfigV1& config) {
    std::string blob;
    blob.reserve(8);
    appendLE<uint32_t>(blob, DummyDeviceConfigV1::magicKey);
    appendLE<uint8_t>(blob, config.enabled ? 1U : 0U);
    appendLE<uint8_t>(blob, config.restorePreviousState ? 1U : 0U);
    appendLE<uint8_t>(blob, config.defaultOutput ? 1U : 0U);
    appendLE<uint8_t>(blob, config.currentOutput ? 1U : 0U);
    return blob;
}

std::string encodeDummyDeviceConfig(const DummyDeviceConfigV2& config) {
    std::string blob;
    blob.reserve(16);
    appendLE<uint32_t>(blob, DummyDeviceConfigV2::magicKey);
    appendLE<uint8_t>(blob, config.enabled ? 1U : 0U);
    appendLE<uint8_t>(blob, config.restorePreviousState ? 1U : 0U);
    appendLE<uint8_t>(blob, config.defaultOutput ? 1U : 0U);
    appendLE<uint8_t>(blob, config.currentOutput ? 1U : 0U);
    appendLE<uint8_t>(blob, config.inverted ? 1U : 0U);
    appendLE<uint8_t>(blob, 0U);
    appendLE<uint8_t>(blob, 0U);
    appendLE<uint8_t>(blob, 0U);
    return blob;
}

bool decodeDummyDeviceConfig(const std::string& blob, DummyDeviceConfigV2& config) {
    size_t pos = 0;
    uint32_t magic{0};
    uint8_t enabled{0};
    uint8_t restore{0};
    uint8_t defaultOutput{0};
    uint8_t currentOutput{0};
    uint8_t inverted{0};
    uint8_t reserved{0};
    uint8_t reserved2{0};
    uint8_t reserved3{0};
    if (!readLE(blob, pos, magic)) {
        return false;
    }

    if (magic == DummyDeviceConfigV1::magicKey) {
        if (!readLE(blob, pos, enabled) || !readLE(blob, pos, restore) || !readLE(blob, pos, defaultOutput) ||
            !readLE(blob, pos, currentOutput)) {
            return false;
        }
        config.enabled = enabled != 0;
        config.restorePreviousState = restore != 0;
        config.defaultOutput = defaultOutput != 0;
        config.currentOutput = currentOutput != 0;
        config.inverted = false;
        return true;
    }

    if (magic != DummyDeviceConfigV2::magicKey) {
        return false;
    }

    if (!readLE(blob, pos, enabled) || !readLE(blob, pos, restore) || !readLE(blob, pos, defaultOutput) ||
        !readLE(blob, pos, currentOutput) || !readLE(blob, pos, inverted) || !readLE(blob, pos, reserved) ||
        !readLE(blob, pos, reserved2) || !readLE(blob, pos, reserved3)) {
        return false;
    }

    config.enabled = enabled != 0;
    config.restorePreviousState = restore != 0;
    config.defaultOutput = defaultOutput != 0;
    config.currentOutput = currentOutput != 0;
    config.inverted = inverted != 0;
    return true;
}

bool parseDummyDeviceConfigJson(const JsonObjectConst& input, uint32_t configVersion, DummyDeviceConfigV2& config, std::string& error) {
    if (configVersion == 0) {
        configVersion = 2U;
    }

    if (configVersion != 1U && configVersion != 2U) {
        error = "unsupported DummyDevice config version";
        return false;
    }

    config.enabled = input["enabled"] | true;
    config.restorePreviousState = input["restore_previous_state"] | false;
    config.defaultOutput = input["default_output"] | false;
    config.currentOutput = input["current_output"] | config.defaultOutput;
    config.inverted = input["inverted"] | false;
    if (configVersion == 1U) {
        config.inverted = false;
    }
    return true;
}

void writeDummyDeviceConfigJson(const DummyDeviceConfigV2& config, JsonObject output) {
    output["enabled"] = config.enabled;
    output["restore_previous_state"] = config.restorePreviousState;
    output["default_output"] = config.defaultOutput;
    output["current_output"] = config.currentOutput;
    output["inverted"] = config.inverted;
}

} // namespace ewfm
