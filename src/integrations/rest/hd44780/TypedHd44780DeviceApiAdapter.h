#pragma once

#include "devices/display/DisplayTextPlaceholderAst.h"
#include "devices/display/hd44780/Hd44780ChannelConfig.h"
#include "integrations/rest/common/TypedDeviceApiAdapter.h"
#include "integrations/rest/expander/ExpanderApiAdapterSupport.h"

#include <cstdio>

namespace ewfm {

// Shared CRTP skeleton for HD44780-via-port-expander REST adapters (lcd1602, lcd2004, ...),
// mirroring TypedDisplayDeviceApiAdapter's shape for the pixel-display family: parses the one
// port_expander dependency link plus metric_source deps scanned out of every line template
// (preserving the existing link when an update doesn't resend `deps`, so placeholder-referenced
// devices keep their deletion-protection dependency), and validates the reserved channels against
// the target expander's channelCount() and sibling dependents.
//
// `Derived` supplies:
//   static constexpr const char* kDepsRequiredError;
//   static constexpr const char* kInvalidLineError;
//   static constexpr const char* kDependencyCountError;
//   static bool decodeConfig(const uint8_t* blob, size_t size, Config& config);
//   static uint8_t lineCount();
//   static const char* lineAt(const Config& config, uint8_t index);
//   static const Hd44780ChannelConfigV1& channelsOf(const Config& config);
template <typename Derived, typename Device, typename Config>
class TypedHd44780DeviceApiAdapter : public TypedDeviceApiAdapter<Derived, Device, Config> {
public:
    bool parseCreateExtras(const JsonObjectConst& input, const JsonObjectConst& configInput, Config& config, DeviceCreateRequest& request,
                           const char*& error) const {
        (void)input;
        if (!parseExpanderDepsField(configInput, request.deps, request.depCount, error, Derived::kDepsRequiredError)) {
            return false;
        }
        return collectLineDependencies(config, request.deps, request.depCount, error);
    }

    bool parseUpdateConfigRequest(const JsonObjectConst& input, IDeviceRuntime& runtime, DeviceConfigUpdateRequest& request,
                                  const char*& error) const override {
        const JsonObjectConst configInput = input["config"].as<JsonObjectConst>();
        if (configInput.isNull()) {
            error = this->kConfigRequiredError;
            return false;
        }

        Config config = static_cast<const Device&>(runtime).config();
        if (!this->parseConfigJson(configInput, config, error)) {
            return false;
        }
        if (!this->configIsValid(config)) {
            error = this->kInvalidConfigError;
            return false;
        }

        request = {};
        request.configVersion = this->adapterConfigVersion();
        if (!assignDeviceBaseConfig(request.baseConfig, this->configName(config), this->configEnabled(config))) {
            error = "device base config is invalid";
            return false;
        }
        if (!this->assignEncodedConfigBlob(config, request.configBlob, error)) {
            return false;
        }

        const bool explicitDepsProvided = !input["deps"].isNull();
        if (explicitDepsProvided) {
            if (!parseExpanderDepsField(input, request.deps, request.depCount, error, Derived::kDepsRequiredError)) {
                return false;
            }
        } else {
            request.deps[0] = DeviceDependencyLink{DeviceRole::PortExpander, runtime.dependencyDeviceId(DeviceRole::PortExpander)};
            request.depCount = 1U;
        }
        if (!collectLineDependencies(config, request.deps, request.depCount, error)) {
            return false;
        }
        request.depsProvided = explicitDepsProvided || request.depCount > 1U;
        return true;
    }

    void writeRuntimeJson(const Device& device, JsonObject runtimeJson) const {
        JsonObject outputJson = runtimeJson.createNestedObject("output");
        char key[8];
        for (uint8_t i = 0; i < Derived::lineCount(); ++i) {
            std::snprintf(key, sizeof(key), "line%u", static_cast<unsigned>(i + 1U));
            outputJson[key] = JsonString(device.renderedLine(i), JsonString::Copied);
        }
    }

    DeviceValidationResult validateCreateRequest(const DeviceCreateRequest& request, const DeviceRegistry& registry) const override {
        Config config{};
        if (!Derived::decodeConfig(request.configBlob.data(), request.configBlob.size(), config)) {
            return {DeviceError::InvalidConfig, this->kInvalidConfigError};
        }
        return validateExpanderDependency(registry, findDependencyIdForRole(request.deps, request.depCount, DeviceRole::PortExpander),
                                          config, nullptr);
    }

    DeviceValidationResult validateUpdateConfigRequest(const IDeviceRuntime& runtime, const DeviceConfigUpdateRequest& request,
                                                       const DeviceRegistry& registry) const override {
        Config config{};
        if (!Derived::decodeConfig(request.configBlob.data(), request.configBlob.size(), config)) {
            return {DeviceError::InvalidConfig, this->kInvalidConfigError};
        }
        const DeviceId expanderDeviceId = request.depsProvided
                                              ? findDependencyIdForRole(request.deps, request.depCount, DeviceRole::PortExpander)
                                              : runtime.dependencyDeviceId(DeviceRole::PortExpander);
        return validateExpanderDependency(registry, expanderDeviceId, config, &runtime);
    }

    DeviceValidationResult validateSetDepsRequest(const IDeviceRuntime& runtime,
                                                  const std::array<DeviceDependencyLink, kMaxDeviceDependencies>& deps, uint8_t depCount,
                                                  const DeviceRegistry& registry) const override {
        uint8_t channels[7]{};
        const uint8_t channelCount = runtime.expanderChannels(channels, 7U);
        return validatePortExpanderDependencyChannels(registry, findDependencyIdForRole(deps, depCount, DeviceRole::PortExpander), channels,
                                                      channelCount, &runtime);
    }

private:
    static bool collectLineDependencies(const Config& config, std::array<DeviceDependencyLink, kMaxDeviceDependencies>& deps,
                                        uint8_t& depCount, const char*& error) {
        for (uint8_t i = 0; i < Derived::lineCount(); ++i) {
            if (!collectTextPlaceholderDeviceIds(Derived::lineAt(config, i), deps, depCount, error, Derived::kInvalidLineError,
                                                 Derived::kDependencyCountError)) {
                return false;
            }
        }
        return true;
    }

    static DeviceValidationResult validateExpanderDependency(const DeviceRegistry& registry, DeviceId expanderDeviceId,
                                                             const Config& config, const IDeviceRuntime* childRuntime) {
        uint8_t channels[7]{};
        const uint8_t channelCount = hd44780ConfigChannels(Derived::channelsOf(config), channels, 7U);
        return validatePortExpanderDependencyChannels(registry, expanderDeviceId, channels, channelCount, childRuntime);
    }
};

} // namespace ewfm
