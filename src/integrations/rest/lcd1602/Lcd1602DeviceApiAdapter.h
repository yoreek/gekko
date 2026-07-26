#pragma once

#include "devices/display/lcd1602/Lcd1602Device.h"
#include "integrations/rest/common/TypedDeviceApiAdapter.h"

namespace ewfm {

class Lcd1602DeviceApiAdapter final : public TypedDeviceApiAdapter<Lcd1602DeviceApiAdapter, Lcd1602Device, Lcd1602DeviceConfigV1> {
public:
    static constexpr const char* kTypeName = "lcd1602";
    static constexpr const char* kDepsRequiredError = "lcd1602 port expander dependency is required";
    static constexpr const char* kInvalidLineError = "lcd1602 line placeholder is invalid";
    static constexpr const char* kDependencyCountError = "lcd1602 exceeds maximum dependency count";

    // Create never has a prior runtime to preserve deps from, so the plain extras hook is enough:
    // parse the client's one port_expander link, then append metric_source links scanned out of
    // line1/line2 (see DisplayTextPlaceholderAst::collectTextPlaceholderDeviceIds).
    bool parseCreateExtras(const JsonObjectConst& input, const JsonObjectConst& configInput, Lcd1602DeviceConfigV1& config,
                           DeviceCreateRequest& request, const char*& error) const;

    // Update needs runtime access (to preserve the existing port_expander link when the client
    // doesn't resend `deps`), which the plain parseUpdateExtras hook doesn't receive -- so this
    // overrides parseUpdateConfigRequest directly, mirroring TypedDisplayDeviceApiAdapter's
    // seed-then-collect-then-recompute-depsProvided shape.
    bool parseUpdateConfigRequest(const JsonObjectConst& input, IDeviceRuntime& runtime, DeviceConfigUpdateRequest& request,
                                  const char*& error) const override;

    void writeRuntimeJson(const Lcd1602Device& device, JsonObject runtimeJson) const;

    DeviceValidationResult validateCreateRequest(const DeviceCreateRequest& request, const DeviceRegistry& registry) const override;
    DeviceValidationResult validateUpdateConfigRequest(const IDeviceRuntime& runtime, const DeviceConfigUpdateRequest& request,
                                                       const DeviceRegistry& registry) const override;
    DeviceValidationResult validateSetDepsRequest(const IDeviceRuntime& runtime,
                                                  const std::array<DeviceDependencyLink, kMaxDeviceDependencies>& deps, uint8_t depCount,
                                                  const DeviceRegistry& registry) const override;
};

} // namespace ewfm
