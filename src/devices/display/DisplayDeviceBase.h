#pragma once

#include "devices/core/DeviceRuntimeBase.h"
#include "devices/display/DisplayLayoutCodec.h"
#include "devices/display/DisplayLayoutStore.h"

#include <ArduinoJson.h>

namespace ewfm {

class DisplayDeviceBase : public DeviceRuntimeBase, public IDevicePersistedState {
public:
    explicit DisplayDeviceBase(PState initialState);
    static PState initialState();

    IDevicePersistedState* persistedStateRuntime() override;
    const IDevicePersistedState* persistedStateRuntime() const override;
    DeviceValidationResult loadPersistedState(DeviceScopedDataStore& store) override;
    DeviceValidationResult savePersistedState(DeviceScopedDataStore& store) const override;
    DeviceValidationResult clearPersistedState(DeviceScopedDataStore& store) override;
    DeviceValidationResult applyPersistedStateUpdate(const uint8_t* data, size_t size) override;
    void writeDeviceJson(JsonObject output) const override;

    const DisplayLayoutRecordV1& layout() const;
    void setLayout(const DisplayLayoutRecordV1& layout);

protected:
    virtual void writeDisplayConfigJson(JsonObject output) const = 0;

    State Idle();
    State Starting();
    State Ready();
    State Reconfiguring();
    State Disabled();
    State Faulted();
    State Deleting();

    DisplayLayoutRecordV1 layout_{};
};

} // namespace ewfm
