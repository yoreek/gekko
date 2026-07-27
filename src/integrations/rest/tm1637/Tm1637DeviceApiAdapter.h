#pragma once

#include "devices/display/DisplayLayoutProfile.h"
#include "devices/display/DisplayLayoutValidator.h"
#include "devices/display/tm1637/Tm1637Device.h"
#include "devices/display/tm1637/Tm1637SegmentCodec.h"
#include "integrations/rest/display/DisplayDeviceApiAdapter.h"

#include <array>

namespace ewfm {

class Tm1637DeviceApiAdapter final : public DisplayDeviceApiAdapter {
public:
    static constexpr const char* kTypeName = "tm1637";
    static constexpr const char* kInvalidConfigError = "tm1637 config is invalid";
    static constexpr const char* kDependencyError = "tm1637 requires clk and dio switch dependencies";
    static constexpr const char* kDependencyCountError = "tm1637 requires exactly two deps";
    static constexpr const char* kInvalidLayoutError = "tm1637 layout is invalid";
    static constexpr const char* kLayoutSizeError = "tm1637 layout exceeds supported size";
    static constexpr const char* kLayoutDependencyCountError = "tm1637 layout exceeds supported dependency count";
    static constexpr const char* kLayoutPlaceholderError = "tm1637 layout placeholder is invalid";

    static const Tm1637DeviceApiAdapter& instance();
    static DisplayLayoutProfile layoutProfile();

    DeviceTypeId typeId() const override;
    const char* typeName() const override;
    uint32_t currentConfigVersion() const override;

    bool parseCreateRequest(const JsonObjectConst& input, DeviceCreateRequest& request, const char*& error) const override;
    bool parseCreatePersistedStateRequest(const JsonObjectConst& input, DeviceCreateRequest& request,
                                          DeviceCreatePersistenceRequest& persistedRequest, const char*& error) const override;
    bool parseUpdateConfigRequest(const JsonObjectConst& input, IDeviceRuntime& runtime, DeviceConfigUpdateRequest& request,
                                  const char*& error) const override;
    DeviceValidationResult validateCreateRequest(const DeviceCreateRequest& request, const DeviceRegistry& registry) const override;
    DeviceValidationResult validateCreateRequest(const DeviceCreateRequest& request, const DeviceCreatePersistenceRequest& persistedRequest,
                                                 const DeviceRegistry& registry) const override;
    DeviceValidationResult validateUpdateConfigRequest(const IDeviceRuntime& runtime, const DeviceConfigUpdateRequest& request,
                                                       const DeviceRegistry& registry) const override;
    DeviceValidationResult validateSetDepsRequest(const IDeviceRuntime& runtime,
                                                  const std::array<DeviceDependencyLink, kMaxDeviceDependencies>& deps, uint8_t depCount,
                                                  const DeviceRegistry& registry) const override;
    void writeDeviceJson(const IDeviceRuntime& runtime, DeviceStatus effectiveStatus, JsonObject output) const override;
    void writeConfigJson(const IDeviceRuntime& runtime, JsonObject config) const override;

private:
    static bool parseDeps(const JsonObjectConst& input, std::array<DeviceDependencyLink, kMaxDeviceDependencies>& deps, uint8_t& depCount,
                          const char*& error, bool required);
    static DeviceValidationResult validateDeps(const DeviceRegistry& registry, const DeviceDependencyLink* deps, uint8_t depCount,
                                               const IDeviceRuntime* ignoreRuntime);
    static DeviceValidationResult validatePersistedLayout(const BoundedBlob<kMaxDisplayLayoutBytes>& blob, bool provided,
                                                          const DeviceRegistry& registry);
    static const Tm1637Device& device(const IDeviceRuntime& runtime);
};

} // namespace ewfm
