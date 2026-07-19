#pragma once

#include "devices/core/ConfigCodec.h"
#include "devices/core/DeviceBaseConfig.h"
#include "integrations/common/DeviceApiAdapter.h"

#include <ArduinoJson.h>

namespace ewfm {

// Shared skeleton for per-family REST adapters. A Derived adapter supplies its identity and
// only overrides the hooks whose behavior genuinely differs:
//
//   static constexpr const char* kTypeName;
//
// Error strings are type-agnostic by default (the request context already names the device
// type); exact strings are part of the observable API contract - see
// test_integrations/rest_adapter_roundtrip.cpp.
// Shadowable static hooks (defaults below): adapterTypeId, adapterConfigVersion (both default to
// Device::descriptor()), configSize/encodeConfig (both default to a single fixed config blob),
// parseConfigJson, configIsValid, configName, configEnabled, currentConfig. Shadowable instance
// hooks: parseCreateExtras/parseUpdateExtras (deps parsing and other per-family request fields)
// and writeConfigJson/writeRuntimeJson.
// The validate*Request virtuals stay ordinary IDeviceApiAdapter overrides on the concrete
// adapter. Hook order replicates the historical hand-written adapters exactly; do not reorder.
template <typename Derived, typename Device, typename Config> class TypedDeviceApiAdapter : public IDeviceApiAdapter {
public:
    static constexpr const char* kConfigRequiredError = "device config is required";
    static constexpr const char* kInvalidConfigError = "device config is invalid";
    static constexpr const char* kEncodeError = "failed to encode device config";

    static const Derived& instance() {
        static const Derived adapter;
        return adapter;
    }

    DeviceTypeId typeId() const override {
        return Derived::adapterTypeId();
    }

    const char* typeName() const override {
        return Derived::kTypeName;
    }

    uint32_t currentConfigVersion() const override {
        return Derived::adapterConfigVersion();
    }

    bool parseCreateRequest(const JsonObjectConst& input, DeviceCreateRequest& request, const char*& error) const override {
        request = {};
        request.typeId = Derived::adapterTypeId();
        request.configVersion = Derived::adapterConfigVersion();

        const JsonObjectConst configInput = input["config"].as<JsonObjectConst>();
        if (configInput.isNull()) {
            error = Derived::kConfigRequiredError;
            return false;
        }

        Config config{};
        if (!Derived::parseConfigJson(configInput, config, error)) {
            return false;
        }
        if (!assignDeviceBaseConfig(request.baseConfig, Derived::configName(config), Derived::configEnabled(config))) {
            error = "device base config is invalid";
            return false;
        }
        if (!derived().parseCreateExtras(input, configInput, config, request, error)) {
            return false;
        }
        if (!Derived::configIsValid(config)) {
            error = Derived::kInvalidConfigError;
            return false;
        }
        return assignEncodedConfigBlob(config, request.configBlob, error);
    }

    bool parseUpdateConfigRequest(const JsonObjectConst& input, IDeviceRuntime& runtime, DeviceConfigUpdateRequest& request,
                                  const char*& error) const override {
        const JsonObjectConst configInput = input["config"].as<JsonObjectConst>();
        if (configInput.isNull()) {
            error = Derived::kConfigRequiredError;
            return false;
        }

        // Seeding from the runtime's current config is what gives partial updates their
        // preserve-unspecified-fields semantics.
        Config config = Derived::currentConfig(runtime);
        if (!Derived::parseConfigJsonForUpdate(configInput, config, error)) {
            return false;
        }
        if (!Derived::configIsValid(config)) {
            error = Derived::kInvalidConfigError;
            return false;
        }

        request = {};
        request.configVersion = Derived::adapterConfigVersion();
        if (!assignDeviceBaseConfig(request.baseConfig, Derived::configName(config), Derived::configEnabled(config))) {
            error = "device base config is invalid";
            return false;
        }
        if (!assignEncodedConfigBlob(config, request.configBlob, error)) {
            return false;
        }
        return derived().parseUpdateExtras(input, configInput, config, request, error);
    }

    void writeDeviceJson(const IDeviceRuntime& runtime, const DeviceStatus effectiveStatus, JsonObject output) const override {
        writeCommonDeviceJson(runtime, effectiveStatus, Derived::kTypeName, output);
        const Device& device = static_cast<const Device&>(runtime);
        derived().writeConfigJson(device, output["config"].as<JsonObject>());
        derived().writeRuntimeJson(device, output["runtime"].as<JsonObject>());
    }

    void writeConfigJson(const IDeviceRuntime& runtime, JsonObject config) const override {
        derived().writeConfigJson(static_cast<const Device&>(runtime), config);
    }

protected:
    static DeviceTypeId adapterTypeId() {
        return Device::descriptor().typeId;
    }

    static uint32_t adapterConfigVersion() {
        return Device::descriptor().currentConfigVersion;
    }

    static size_t configSize(const Config& config) {
        return fixedConfigBlobSize(Config::kMagic, config);
    }

    static bool encodeConfig(const Config& config, uint8_t* blob, size_t capacity) {
        return encodeFixedConfigBlob(Config::kMagic, config, blob, capacity);
    }

    static bool parseConfigJson(const JsonObjectConst& configInput, Config& config, const char*& error) {
        return config.parseJson(configInput, error);
    }

    static bool parseConfigJsonForUpdate(const JsonObjectConst& configInput, Config& config, const char*& error) {
        return Derived::parseConfigJson(configInput, config, error);
    }

    static bool configIsValid(const Config& config) {
        return config.validate().ok();
    }

    static const char* configName(const Config& config) {
        return config.name;
    }

    static bool configEnabled(const Config& config) {
        return config.enabled != 0U;
    }

    static Config currentConfig(const IDeviceRuntime& runtime) {
        return static_cast<const Device&>(runtime).config();
    }

    bool parseCreateExtras(const JsonObjectConst& input, const JsonObjectConst& configInput, Config& config, DeviceCreateRequest& request,
                           const char*& error) const {
        (void)input;
        (void)configInput;
        (void)config;
        (void)request;
        (void)error;
        return true;
    }

    bool parseUpdateExtras(const JsonObjectConst& input, const JsonObjectConst& configInput, Config& config,
                           DeviceConfigUpdateRequest& request, const char*& error) const {
        (void)input;
        (void)configInput;
        (void)config;
        (void)request;
        (void)error;
        return true;
    }

    void writeConfigJson(const Device& device, JsonObject config) const {
        device.config().writeJson(config);
    }

    void writeRuntimeJson(const Device& device, JsonObject runtimeJson) const {
        (void)device;
        (void)runtimeJson;
    }

    template <typename Blob> static bool assignEncodedConfigBlob(const Config& config, Blob& blob, const char*& error) {
        uint8_t buffer[kMaxDeviceConfigBytes]{};
        const size_t size = Derived::configSize(config);
        if (!Derived::encodeConfig(config, buffer, size) || !blob.assign(buffer, size)) {
            error = Derived::kEncodeError;
            return false;
        }
        return true;
    }

private:
    const Derived& derived() const {
        return static_cast<const Derived&>(*this);
    }
};

} // namespace ewfm
