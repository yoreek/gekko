#pragma once

#include "devices/expander/PortExpanderConfig.h"
#include "integrations/rest/common/I2cDeviceApiSupport.h"
#include "integrations/rest/common/TypedDeviceApiAdapter.h"
#include "integrations/rest/expander/ExpanderApiAdapterSupport.h"

namespace ewfm {

// PCF8574 and PCF8575 share the Pcf857xExpanderConfigV2 codec and differ only in device class
// and type name, so a Derived adapter supplies only kTypeName.
template <typename Derived, typename Device>
class Pcf857xExpanderApiAdapterBase : public TypedDeviceApiAdapter<Derived, Device, Pcf857xExpanderConfigV2> {
public:
    static constexpr const char* kDepsRequiredError = "pcf857x expander deps are required";

    bool parseCreateExtras(const JsonObjectConst& input, const JsonObjectConst& configInput, Pcf857xExpanderConfigV2& config,
                           DeviceCreateRequest& request, const char*& error) const {
        (void)input;
        (void)config;
        return parseExpanderDepsField(configInput, request.deps, request.depCount, error, Derived::kDepsRequiredError);
    }

    bool parseUpdateExtras(const JsonObjectConst& input, const JsonObjectConst& configInput, Pcf857xExpanderConfigV2& config,
                           DeviceConfigUpdateRequest& request, const char*& error) const {
        (void)configInput;
        (void)config;
        request.depsProvided = !input["deps"].isNull();
        if (request.depsProvided && !parseExpanderDepsField(input, request.deps, request.depCount, error, Derived::kDepsRequiredError)) {
            return false;
        }
        return true;
    }

    void writeRuntimeJson(const Device& device, JsonObject runtimeJson) const {
        runtimeJson["channelCount"] = device.channelCount();
        runtimeJson["channelStates"] = device.channelStatesBitmask();
        device.diagnostics().writeJson(runtimeJson);
    }

    DeviceValidationResult validateCreateRequest(const DeviceCreateRequest& request, const DeviceRegistry& registry) const override {
        Pcf857xExpanderConfigV2 config{};
        if (!decodePcf857xExpanderConfig(reinterpret_cast<const uint8_t*>(request.configBlob.data()), request.configBlob.size(), config)) {
            return {DeviceError::InvalidConfig, Derived::kInvalidConfigError};
        }
        return validateI2cBusDependency(registry, findDependencyIdForRole(request.deps, request.depCount, DeviceRole::I2CBus),
                                        config.i2cAddress, nullptr);
    }

    DeviceValidationResult validateUpdateConfigRequest(const IDeviceRuntime& runtime, const DeviceConfigUpdateRequest& request,
                                                       const DeviceRegistry& registry) const override {
        Pcf857xExpanderConfigV2 config{};
        if (!decodePcf857xExpanderConfig(reinterpret_cast<const uint8_t*>(request.configBlob.data()), request.configBlob.size(), config)) {
            return {DeviceError::InvalidConfig, Derived::kInvalidConfigError};
        }
        const DeviceId dependencyDeviceId = request.depsProvided
                                                ? findDependencyIdForRole(request.deps, request.depCount, DeviceRole::I2CBus)
                                                : runtime.dependencyDeviceId(DeviceRole::I2CBus);
        return validateI2cBusDependency(registry, dependencyDeviceId, config.i2cAddress, &runtime);
    }

    DeviceValidationResult validateSetDepsRequest(const IDeviceRuntime& runtime,
                                                  const std::array<DeviceDependencyLink, kMaxDeviceDependencies>& deps, uint8_t depCount,
                                                  const DeviceRegistry& registry) const override {
        uint8_t address = 0;
        if (!runtime.i2cAddress(address)) {
            return {DeviceError::InvalidConfig, Derived::kInvalidConfigError};
        }
        return validateI2cBusDependency(registry, findDependencyIdForRole(deps, depCount, DeviceRole::I2CBus), address, &runtime);
    }
};

} // namespace ewfm
