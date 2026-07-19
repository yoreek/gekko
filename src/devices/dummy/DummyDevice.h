#pragma once

#include "devices/core/DeviceBaseConfig.h"
#include "devices/core/DeviceRuntimeBase.h"
#include "devices/core/DeviceTypes.h"

#include <cstddef>
#include <cstdint>

namespace ewfm {

using DummyDeviceConfigV1 = DeviceBaseConfigV1;

constexpr size_t dummyDeviceConfigSize(const DummyDeviceConfigV1&) {
    return deviceBaseConfigSize(DeviceBaseConfigV1{});
}

class DummyDevice final : public DeviceRuntimeBase {
public:
    DummyDevice(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob);

    const DummyDeviceConfigV1& config() const;
    bool deleted() const;
    bool serializeConfigBlob(DeviceConfigBlob& configBlob) const override;
    DeviceConfigUpdatePlan planConfigUpdate(const DeviceConfigBlob& configBlob) const override;
    bool applyConfig(const DeviceConfigBlob& configBlob, uint32_t now) override;

    static DeviceTypeDescriptor descriptor();
    static std::unique_ptr<IDeviceRuntime> createRuntime(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob);
    static DeviceValidationResult validateConfig(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob);

private:
    const DeviceBaseConfigV1& baseConfig() const override;

    State Idle();
    State Starting();
    State Ready();
    State Reconfiguring();
    State DependencyBlocked();
    State Disabled();
    State Faulted();
    State Deleting();

    DummyDeviceConfigV1 config_{};
};

} // namespace ewfm
