#pragma once

#include "core/StateMachine.h"
#include "devices/core/DeviceTypes.h"

#include <ArduinoJson.h>
#include <string>

namespace ewfm {

#pragma pack(push, 1)
struct DummyDeviceConfigV1 {
    static constexpr uint32_t kMagicKey = 0x44554D31UL;
    uint8_t enabled{1};
    uint8_t restorePreviousState{0};
    uint8_t defaultOutput{0};
    uint8_t currentOutput{0};
};

struct DummyDeviceConfigV2 {
    static constexpr uint32_t kMagicKey = 0x44554D32UL;
    uint8_t enabled{1};
    uint8_t restorePreviousState{0};
    uint8_t defaultOutput{0};
    uint8_t currentOutput{0};
    uint8_t inverted{0};

    void migrateFrom(const DummyDeviceConfigV1& orig);
    void migrateFrom(const DummyDeviceConfigV2& orig);
};
#pragma pack(pop)

std::string encodeDummyDeviceConfig(const DummyDeviceConfigV1& config);
std::string encodeDummyDeviceConfig(const DummyDeviceConfigV2& config);
bool decodeDummyDeviceConfig(const std::string& blob, DummyDeviceConfigV2& config);
bool parseDummyDeviceConfigJson(const JsonObjectConst& input, uint32_t configVersion, DummyDeviceConfigV2& config, std::string& error);
void writeDummyDeviceConfigJson(const DummyDeviceConfigV2& config, JsonObject output);

class DummyDevice final : public StateMachine, public IDeviceRuntime {
public:
    explicit DummyDevice(const DeviceRecord& record);

    void begin(uint32_t now) override;
    void tickFastLoop(uint32_t now) override;
    void tick100ms(uint32_t now) override;
    void tick1s(uint32_t now) override;
    void setParentRuntime(IDeviceRuntime* parentRuntime) override;
    IDeviceRuntime* parentRuntime() const override;
    void attachChildRuntime(IDeviceRuntime* childRuntime) override;
    void detachChildRuntime(IDeviceRuntime* childRuntime) override;
    const std::vector<IDeviceRuntime*>& childRuntimes() const override;
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
    State DependencyBlocked();
    State Disabled();
    State Faulted();
    State Deleting();

    bool parentReady() const;
    bool hasChildRuntime(const IDeviceRuntime* childRuntime) const;

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
    IDeviceRuntime* parentRuntime_{nullptr};
    std::vector<IDeviceRuntime*> childRuntimes_{};
};

} // namespace ewfm
