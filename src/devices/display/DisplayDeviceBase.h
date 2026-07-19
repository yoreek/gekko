#pragma once

#include "devices/core/DeviceRuntimeBase.h"
#include "devices/display/DisplayLayoutCodec.h"
#include "devices/display/DisplayLayoutRenderer.h"
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

    const DisplayLayoutRecordV1& layout() const;
    void setLayout(const DisplayLayoutRecordV1& layout);
    bool renderDisplay(const MetricValueResolver& resolver, uint32_t now);
    DisplayDeviceBase* displayRuntime() override;
    const DisplayDeviceBase* displayRuntime() const override;

protected:
    virtual void writeDisplayConfigJson(JsonObject output) const = 0;
    virtual bool initializeDisplayHardware(uint32_t now);
    virtual void releaseDisplayHardware(uint32_t now);
    virtual IDisplayRenderSurface* renderSurface() const;
    virtual void onDisplayFrameRendered(const DisplayLayoutRenderResult& result);
    void invalidateDisplayRender();

    State Idle();
    State Starting();
    State Ready();
    State Reconfiguring();
    State Disabled();
    State Faulted();
    State Deleting();

    DisplayLayoutRecordV1 layout_{};
    DisplayLayoutRenderSession renderSession_{};
    bool emptyLayoutCleared_{false};
};

} // namespace ewfm
