#pragma once

#include "devices/switch/SwitchOutputState.h"
#include "devices/switch/expander/PortExpanderSwitchDevice.h"
#include "integrations/rest/common/TypedDeviceApiAdapter.h"
#include "integrations/rest/expander/ExpanderApiAdapterSupport.h"

namespace ewfm {

class PortExpanderSwitchDeviceApiAdapter final
    : public TypedDeviceApiAdapter<PortExpanderSwitchDeviceApiAdapter, PortExpanderSwitchDevice, PortExpanderSwitchDeviceConfigV3> {
public:
    static constexpr const char* kTypeName = "port_expander_switch";
    static constexpr const char* kDepsRequiredError = "port expander switch deps are required";

    // Historically this family validated the config BEFORE parsing deps (the other dep-having
    // adapters do the opposite), so a request that is invalid in both ways reports the config
    // error. Validate here first to keep that error precedence; the skeleton's own validity
    // check after this hook is then a no-op re-check.
    bool parseCreateExtras(const JsonObjectConst& input, const JsonObjectConst& configInput, PortExpanderSwitchDeviceConfigV3& config,
                           DeviceCreateRequest& request, const char*& error) const {
        (void)input;
        if (!config.validate().ok()) {
            error = kInvalidConfigError;
            return false;
        }
        return parseExpanderDepsField(configInput, request.deps, request.depCount, error, kDepsRequiredError);
    }

    bool parseUpdateExtras(const JsonObjectConst& input, const JsonObjectConst& configInput, PortExpanderSwitchDeviceConfigV3& config,
                           DeviceConfigUpdateRequest& request, const char*& error) const {
        (void)configInput;
        (void)config;
        request.depsProvided = !input["deps"].isNull();
        if (request.depsProvided && !parseExpanderDepsField(input, request.deps, request.depCount, error, kDepsRequiredError)) {
            return false;
        }
        return true;
    }

    void writeRuntimeJson(const PortExpanderSwitchDevice& device, JsonObject runtimeJson) const {
        JsonObject outputJson = runtimeJson.createNestedObject("output");
        outputJson["state"] = device.currentOutputState();
        outputJson["physicalLevel"] = device.physicalOutputState();
    }

    DeviceValidationResult validateCreateRequest(const DeviceCreateRequest& request, const DeviceRegistry& registry) const override;
    DeviceValidationResult validateUpdateConfigRequest(const IDeviceRuntime& runtime, const DeviceConfigUpdateRequest& request,
                                                       const DeviceRegistry& registry) const override;
    DeviceValidationResult validateSetDepsRequest(const IDeviceRuntime& runtime,
                                                  const std::array<DeviceDependencyLink, kMaxDeviceDependencies>& deps, uint8_t depCount,
                                                  const DeviceRegistry& registry) const override;
};

} // namespace ewfm
