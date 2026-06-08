#pragma once

#include "core/StateMachine.h"
#include "devices/DeviceTypes.h"

namespace ewfm {

struct DummyDeviceConfigV1 {
    static constexpr uint32_t magicKey = 0x44554D31UL;
    bool enabled{true};
    bool restorePreviousState{false};
    bool defaultOutput{false};
    bool currentOutput{false};
};

struct DummyDeviceConfigV2 {
    static constexpr uint32_t magicKey = 0x44554D32UL;
    bool enabled{true};
    bool restorePreviousState{false};
    bool defaultOutput{false};
    bool currentOutput{false};
    bool inverted{false};

    void migrateFrom(const DummyDeviceConfigV1& orig);
};

class DummyDevice final : public StateMachine, public IDeviceRuntime {
public:
    explicit DummyDevice(const DeviceRecord& record);

    void begin(uint32_t now) override;
    void tickFastLoop(uint32_t now) override;
    void tick100ms(uint32_t now) override;
    void tick1s(uint32_t now) override;
    void requestReconfigure() override;
    void requestDisable() override;
    void requestDelete() override;
    DeviceStatus status() const override;
    bool handleCommand(const DeviceCommand& command) override;

    void applyRetainedState(bool output);
    bool outputState() const;
    bool restorePreviousState() const;
    const DummyDeviceConfigV2& config() const;
    bool deleted() const;

    static DeviceTypeDescriptor descriptor();
    static std::unique_ptr<IDeviceRuntime> createRuntime(const DeviceRecord& record);
    static DeviceValidationResult validateConfig(const DeviceRecord& record);

private:
    void tickCadence(uint32_t now);

    State Idle();
    State Starting();
    State Ready();
    State Reconfiguring();
    State Disabled();
    State Faulted();
    State Deleting();

    DummyDeviceConfigV2 config_{};
    DeviceStatus status_{DeviceStatus::Unknown};
    bool startRequested_{false};
    bool reconfigureRequested_{false};
    bool disableRequested_{false};
    bool deleteRequested_{false};
    bool faultRequested_{false};
    bool retainedStateAvailable_{false};
    bool retainedOutput_{false};
    bool deleted_{false};
};

} // namespace ewfm
