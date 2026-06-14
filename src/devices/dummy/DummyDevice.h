#pragma once

#include "devices/core/DeviceRuntimeBase.h"
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

class DummyDevice final : public DeviceRuntimeBase {
public:
    explicit DummyDevice(const DeviceRecord& record);

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
    State Idle();
    State Starting();
    State Ready();
    State Reconfiguring();
    State DependencyBlocked();
    State Disabled();
    State Faulted();
    State Deleting();

    DummyDeviceConfigV2 config_{};
    bool retainedStateAvailable_{false};
    bool retainedOutput_{false};
};

} // namespace ewfm
