#pragma once

#include "devices/analog/composer/AnalogOutputComposerDeviceConfig.h"
#include "devices/core/DeviceRuntimeBase.h"

namespace ewfm {

#pragma pack(push, 1)
struct AnalogOutputComposerRetainedStateV1 {
    uint16_t recordVersion{kRetainedStateRecordVersion};
    DeviceId deviceId{0U};
    uint8_t mode{static_cast<uint8_t>(AnalogOutputMode::Scheduled)};
};
#pragma pack(pop)

class AnalogOutputComposerDevice final : public DeviceRuntimeBase, public IAnalogOutputGroupRuntime {
public:
    AnalogOutputComposerDevice(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob);
    explicit AnalogOutputComposerDevice(const AnalogOutputComposerDeviceConfigV1& config);

    const AnalogOutputComposerDeviceConfigV1& config() const;
    const IAnalogOutputGroupRuntime* analogOutputGroupRuntime() const override {
        return this;
    }

    AnalogOutputMode analogOutputGroupMode() const override;
    bool requestAnalogOutputGroupMode(AnalogOutputMode mode, uint32_t now) override;
    bool handleCommand(const DeviceCommand& command) override;

    bool serializeConfigBlob(DeviceConfigBlob& configBlob) const override;
    DeviceConfigUpdatePlan planConfigUpdate(const DeviceConfigBlob& configBlob) const override;
    bool applyConfig(const DeviceConfigBlob& configBlob, uint32_t now) override;
    bool retainedStateDirty() const override;
    void clearRetainedStateDirty() override;
    DeviceValidationResult saveRetainedState(DeviceRetainedDataStore& store) const override;
    DeviceValidationResult loadRetainedState(DeviceRetainedDataStore& store) override;

    static DeviceTypeDescriptor descriptor();
    static std::unique_ptr<IDeviceRuntime> createRuntime(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob);
    static DeviceValidationResult validateConfig(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob);

private:
    const DeviceBaseConfigV1& baseConfig() const override;
    State Idle();
    State Starting();
    State Reconfiguring();
    State Ready();
    State DependencyBlocked();
    State Disabled();
    State Deleting();
    IAnalogOutputRuntime* outputAt(uint8_t index) const;
    IScheduledAnalogOutputRuntime* scheduledOutputAt(uint8_t index) const;
    bool applyModeToOutputs(AnalogOutputMode mode, uint32_t now) const;
    bool outputsReady() const;
    void setMode(AnalogOutputMode mode);

    AnalogOutputComposerDeviceConfigV1 config_{};
    AnalogOutputMode mode_{AnalogOutputMode::Scheduled};
    bool retainedStateDirty_{false};
};

} // namespace ewfm
