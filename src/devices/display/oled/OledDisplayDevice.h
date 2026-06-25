#pragma once

#include "devices/core/DeviceRuntimeBase.h"
#include "devices/display/oled/OledDisplayDeviceConfig.h"
#include "devices/display/oled/OledDisplayLayoutStore.h"

#include <ArduinoJson.h>

namespace ewfm {

class OledDisplayDevice final : public DeviceRuntimeBase, public IDevicePersistedState {
public:
    OledDisplayDevice(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob);

    const OledDisplayDeviceConfigV1& config() const;
    bool enabled() const override;
    const char* name() const override;
    bool serializeConfigBlob(DeviceConfigBlob& configBlob) const override;
    bool replaceBaseConfig(DeviceConfigBlob& configBlob, const DeviceBaseConfigV1& baseConfig) const override;
    DeviceConfigUpdatePlan planConfigUpdate(const DeviceConfigBlob& configBlob) const override;
    bool applyConfig(const DeviceConfigBlob& configBlob, uint32_t now) override;
    IDevicePersistedState* persistedStateRuntime() override;
    const IDevicePersistedState* persistedStateRuntime() const override;
    DeviceValidationResult loadPersistedState(DeviceScopedDataStore& store) override;
    DeviceValidationResult savePersistedState(DeviceScopedDataStore& store) const override;
    DeviceValidationResult clearPersistedState(DeviceScopedDataStore& store) override;
    DeviceValidationResult applyPersistedStateUpdate(const uint8_t* data, size_t size) override;
    void setLayout(const OledDisplayLayoutRecordV1& layout);
    void writeDeviceJson(JsonObject output) const;
    const OledDisplayLayoutRecordV1& layout() const;

    static DeviceTypeDescriptor descriptor();
    static std::unique_ptr<IDeviceRuntime> createRuntime(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob);
    static DeviceValidationResult validateConfig(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob);

private:
    State Idle();
    State Starting();
    State Ready();
    State Reconfiguring();
    State Disabled();
    State Faulted();
    State Deleting();

    OledDisplayDeviceConfigV1 config_{};
    OledDisplayLayoutRecordV1 layout_{};
};

} // namespace ewfm
