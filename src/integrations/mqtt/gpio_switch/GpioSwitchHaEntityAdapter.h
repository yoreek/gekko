#pragma once

#include "integrations/mqtt/HaEntityAdapter.h"

namespace ewfm {

class GpioSwitchHaEntityAdapter final : public IHaEntityAdapter {
public:
    static const GpioSwitchHaEntityAdapter& instance();

    DeviceTypeId typeId() const override;
    const char* typeName() const override;
    const char* haComponent() const override;
    void buildDiscoveryPayload(const IDeviceRuntime& runtime, const std::string& uniqueId, const std::string& effectiveName,
                               const std::string& stateTopic, const std::string& commandTopic, JsonObject output) const override;
    bool buildStatePayload(const IDeviceRuntime& runtime, std::string& payload) const override;
    bool parseCommand(const std::string& payload, DeviceId deviceId, DeviceCommand& command) const override;
};

} // namespace ewfm
