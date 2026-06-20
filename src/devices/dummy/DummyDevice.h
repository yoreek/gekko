#pragma once

#include "devices/core/DeviceBaseConfig.h"
#include "devices/core/DeviceRuntimeBase.h"
#include "devices/core/DeviceTypes.h"

#include <ArduinoJson.h>
#include <cstddef>
#include <cstdint>

namespace ewfm {

using DummyDeviceConfigV1 = DeviceBaseConfigV1;

constexpr size_t dummyDeviceConfigSize(const DummyDeviceConfigV1&) {
    return deviceBaseConfigSize(DeviceBaseConfigV1{});
}

bool encodeDummyDeviceConfig(const DummyDeviceConfigV1& config, uint8_t* blob, size_t capacity);
bool decodeDummyDeviceConfig(const uint8_t* blob, size_t size, DummyDeviceConfigV1& config);
bool parseDummyDeviceConfigJson(const JsonObjectConst& input, uint32_t configVersion, DummyDeviceConfigV1& config, const char*& error);
void writeDummyDeviceConfigJson(const DummyDeviceConfigV1& config, JsonObject output);

class DummyDevice final : public DeviceRuntimeBase {
public:
    DummyDevice(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob);

    const DummyDeviceConfigV1& config() const;
    bool deleted() const;
    bool serializeConfigBlob(DeviceConfigBlob& configBlob) const override;
    bool replaceBaseConfig(DeviceConfigBlob& configBlob, const DeviceBaseConfigV1& baseConfig) const override;
    void writeDeviceJson(JsonObject output) const;

    static DeviceTypeDescriptor descriptor();
    static std::unique_ptr<IDeviceRuntime> createRuntime(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob);
    static DeviceValidationResult validateConfig(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob);

private:
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
