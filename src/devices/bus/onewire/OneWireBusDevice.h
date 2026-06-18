#pragma once

#include "devices/bus/onewire/IOneWireBusDriver.h"
#include "devices/bus/onewire/OneWireBusConfig.h"
#include "devices/core/DeviceRuntimeBase.h"

namespace ewfm {

constexpr size_t kMaxOneWireScanDevices = 16;

struct OneWireScanResult {
    OneWireRomAddress devices[kMaxOneWireScanDevices]{};
    uint8_t deviceCount{0};
    bool inProgress{false};
    bool ready{false};
    bool truncated{false};
    bool invalidCandidateSeen{false};
};

class OneWireBusDevice final : public DeviceRuntimeBase {
public:
    explicit OneWireBusDevice(const DeviceRecord& record);
    OneWireBusDevice(const OneWireBusDeviceConfigV1& config, IOneWireBusDriver& driver);

    const OneWireBusDeviceConfigV1& config() const;
    const OneWireScanResult& scan() const;

    static DeviceTypeDescriptor descriptor();
    static std::unique_ptr<IDeviceRuntime> createRuntime(const DeviceRecord& record);
    static DeviceValidationResult validateConfig(const DeviceRecord& record);

    bool handleCommand(const DeviceCommand& command) override;

private:
    State Idle();
    State Starting();
    State Ready();
    State Scanning();
    State Reconfiguring();
    State Disabled();
    State Faulted();
    State Deleting();
    State DependencyBlocked();

    void resetScanResult();
    bool hasVisibleScanState() const;
    void startScan();
    void finishScan();
    void appendScanCandidate(const OneWireRomAddress& address);
    void releaseHardware();
    DeviceValidationResult initializeHardware(uint32_t now);
    bool isScanning() const;

    OneWireBusDeviceConfigV1 config_{};
    IOneWireBusDriver& driver_;
    OneWireScanResult scan_{};
};

} // namespace ewfm
