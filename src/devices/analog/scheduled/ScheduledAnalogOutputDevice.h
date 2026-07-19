#pragma once

#include "devices/analog/AnalogOutputDecoratorDeviceBase.h"
#include "devices/analog/scheduled/ScheduledAnalogOutputDeviceConfig.h"

namespace ewfm {

#pragma pack(push, 1)
struct ScheduledAnalogOutputRetainedStateV1 {
    uint16_t recordVersion{kRetainedStateRecordVersion};
    DeviceId deviceId{0U};
    uint8_t mode{static_cast<uint8_t>(AnalogOutputMode::Scheduled)};
    uint16_t manualState{0U};
};
#pragma pack(pop)

class ScheduledAnalogOutputDevice final : public AnalogOutputDecoratorDeviceBase, public IScheduledAnalogOutputRuntime {
public:
    ScheduledAnalogOutputDevice(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob);
    explicit ScheduledAnalogOutputDevice(const ScheduledAnalogOutputDeviceConfigV2& config);

    const ScheduledAnalogOutputDeviceConfigV2& config() const;
    const IScheduledAnalogOutputRuntime* scheduledAnalogOutputRuntime() const override {
        return this;
    }
    AnalogOutputMode analogOutputMode() const override;
    bool requestAnalogOutputMode(AnalogOutputMode mode, uint32_t now) override;
    uint16_t requestedAnalogOutputState() const override;
    bool analogOutputTimeValid() const override;
    bool requestOutputState(uint16_t state, uint32_t now) override;
    bool handleCommand(const DeviceCommand& command) override;

    bool serializeConfigBlob(DeviceConfigBlob& configBlob) const override;
    DeviceConfigUpdatePlan planConfigUpdate(const DeviceConfigBlob& configBlob) const override;
    bool applyConfig(const DeviceConfigBlob& configBlob, uint32_t now) override;
    bool retainedStateDirty() const override;
    void clearRetainedStateDirty() override;
    DeviceValidationResult saveRetainedState(DeviceRetainedDataStore& store) const override;
    DeviceValidationResult loadRetainedState(DeviceRetainedDataStore& store) override;

    static uint16_t scheduledStateAt(const ScheduledAnalogOutputDeviceConfigV2& config, uint16_t minuteOfDay, bool& hasPoints);
    static DeviceTypeDescriptor descriptor();
    static std::unique_ptr<IDeviceRuntime> createRuntime(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob);
    static DeviceValidationResult validateConfig(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob);

private:
    const DeviceBaseConfigV1& baseConfig() const override;
    void onReadyTick(uint32_t now) override;
    void onTargetAttached(uint32_t now) override;
    void updateScheduledState(uint32_t now, bool force);
    void setMode(AnalogOutputMode mode);

    ScheduledAnalogOutputDeviceConfigV2 config_{};
    AnalogOutputMode mode_{AnalogOutputMode::Scheduled};
    uint16_t manualState_{0U};
    uint16_t scheduledState_{0U};
    uint32_t cachedMinuteKey_{0U};
    bool timeValid_{false};
    bool cacheValid_{false};
    bool retainedStateDirty_{false};
};

} // namespace ewfm
