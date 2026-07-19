#pragma once

#include "devices/analog/AnalogOutputDeviceConfig.h"
#include "integrations/rest/common/TypedDeviceApiAdapter.h"

namespace ewfm {

template <typename Derived, typename Device, typename Config, uint8_t MaxOutputs>
class TypedAnalogOutputDependentApiAdapter : public TypedDeviceApiAdapter<Derived, Device, Config> {
public:
    bool parseCreateExtras(const JsonObjectConst& input, const JsonObjectConst& configInput, Config& config, DeviceCreateRequest& request,
                           const char*& error) const {
        (void)input;
        (void)config;
        return parseOutputDeps(configInput, request.deps, request.depCount, error);
    }

    bool parseUpdateExtras(const JsonObjectConst& input, const JsonObjectConst& configInput, Config& config,
                           DeviceConfigUpdateRequest& request, const char*& error) const {
        (void)configInput;
        (void)config;
        request.depsProvided = !input["deps"].isNull();
        return !request.depsProvided || parseOutputDeps(input, request.deps, request.depCount, error);
    }

    bool parseSetDepsRequest(const JsonObjectConst& input, std::array<DeviceDependencyLink, kMaxDeviceDependencies>& deps,
                             uint8_t& depCount, const char*& error) const override {
        return parseOutputDeps(input, deps, depCount, error);
    }

private:
    static bool parseOutputDeps(const JsonObjectConst& input, std::array<DeviceDependencyLink, kMaxDeviceDependencies>& deps,
                                uint8_t& depCount, const char*& error) {
        if (!IDeviceApiAdapter::parseDependenciesJson(input, deps, depCount, error)) {
            return false;
        }
        if (depCount == 0U || depCount > MaxOutputs) {
            error =
                MaxOutputs == 1U ? "exactly one analog output dependency is required" : "analog output dependency count is out of bounds";
            return false;
        }
        for (uint8_t index = 0U; index < depCount; ++index) {
            if (deps[index].role != DeviceRole::AnalogOutput) {
                error = "dependency role must be analog_output";
                return false;
            }
            for (uint8_t other = static_cast<uint8_t>(index + 1U); other < depCount; ++other) {
                if (deps[index].deviceId == deps[other].deviceId) {
                    error = "analog output dependency is duplicated";
                    return false;
                }
            }
        }
        return true;
    }
};

} // namespace ewfm
