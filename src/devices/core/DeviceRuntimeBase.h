#pragma once

#include "core/StateMachine.h"
#include "devices/core/DeviceTypes.h"

#include <vector>

namespace ewfm {

class DeviceRuntimeBase : public StateMachine, public IDeviceRuntime {
public:
    explicit DeviceRuntimeBase(PState initialState);

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

protected:
    void tickRuntime(uint32_t now);
    void setStatus(DeviceStatus status);
    bool parentReady() const;
    bool hasChildRuntime(const IDeviceRuntime* childRuntime) const;
    bool startRequested() const;
    bool reconfigureRequested() const;
    bool disableRequested() const;
    bool deleteRequested() const;
    bool faultRequested() const;
    void requestFault();
    void clearFaultRequested();
    void clearStartRequested();
    void clearReconfigureRequested();
    void clearDisableRequested();
    void clearDeleteRequested();
    void setDeleted();

    DeviceStatus status_{DeviceStatus::Unknown};
    bool startRequested_{false};
    bool reconfigureRequested_{false};
    bool disableRequested_{false};
    bool deleteRequested_{false};
    bool faultRequested_{false};
    bool deleted_{false};

private:
    IDeviceRuntime* parentRuntime_{nullptr};
    std::vector<IDeviceRuntime*> childRuntimes_{};
};

} // namespace ewfm
