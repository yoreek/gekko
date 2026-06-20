#pragma once

#include "devices/bus/onewire/IOneWireBusDriver.h"
#include "devices/bus/onewire/OneWireBusConfig.h"
#include "devices/core/DeviceRuntimeBase.h"

#include <ArduinoJson.h>

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
    class DependencyTransaction {
    public:
        DependencyTransaction() = default;
        DependencyTransaction(OneWireBusDevice* bus, IOneWireBusDriver* driver, uint32_t generation);
        DependencyTransaction(const DependencyTransaction&) = delete;
        DependencyTransaction& operator=(const DependencyTransaction&) = delete;
        DependencyTransaction(DependencyTransaction&& other) noexcept;
        DependencyTransaction& operator=(DependencyTransaction&& other) noexcept;
        ~DependencyTransaction();

        explicit operator bool() const;
        IOneWireBusDriver* driver() const;
        uint32_t generation() const;
        void release();

    private:
        OneWireBusDevice* bus_{nullptr};
        IOneWireBusDriver* driver_{nullptr};
        uint32_t generation_{0};
    };

    OneWireBusDevice(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob);
    OneWireBusDevice(const OneWireBusDeviceConfigV1& config, IOneWireBusDriver& driver);

    const OneWireBusDeviceConfigV1& config() const;
    const OneWireScanResult& scan() const;
    uint32_t generation() const;
    bool dependencyTransactionActive() const;
    DependencyTransaction beginDependencyTransaction();
    void bindDeviceIdentity(const DeviceRegistryEntry& record, const DeviceConfigBlob& config) override;
    bool serializeConfigBlob(DeviceConfigBlob& configBlob) const override;
    bool replaceBaseConfig(DeviceConfigBlob& configBlob, const DeviceBaseConfigV1& baseConfig) const override;
    void writeDeviceJson(JsonObject output) const;

    static DeviceTypeDescriptor descriptor();
    static std::unique_ptr<IDeviceRuntime> createRuntime(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob);
    static DeviceValidationResult validateConfig(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob);

    bool handleCommand(const DeviceCommand& command) override;
    bool hasDuplicateDependentRomAddress(const OneWireRomAddress& address, const IDeviceRuntime* ignoreDependent = nullptr) const override;

private:
    OneWireBusDevice(const OneWireBusDeviceConfigV1& config, std::unique_ptr<IOneWireBusDriver> ownedDriver);

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
    void releaseDependencyTransaction();
    DeviceValidationResult initializeHardware(uint32_t now);
    bool isScanning() const;

    OneWireBusDeviceConfigV1 config_{};
    std::unique_ptr<IOneWireBusDriver> ownedDriver_{};
    IOneWireBusDriver& driver_;
    OneWireScanResult scan_{};
    uint32_t generation_{0};
    bool dependencyTransactionActive_{false};
};

} // namespace ewfm
