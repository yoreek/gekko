#pragma once

#include "devices/bus/i2c/I2cBusConfig.h"
#include "devices/bus/i2c/II2cBusDriver.h"
#include "devices/bus/BusRuntimeDiagnostics.h"
#include "devices/core/DeviceRuntimeBase.h"

#include <ArduinoJson.h>

namespace ewfm {

constexpr size_t kMaxI2cScanDevices = 16;
constexpr uint8_t kI2cScanFirstAddress = 0x08U;
constexpr uint8_t kI2cScanLastAddress = 0x77U;

struct I2cScanResult {
    uint8_t devices[kMaxI2cScanDevices]{};
    uint8_t deviceCount{0};
    uint8_t nextAddress{kI2cScanFirstAddress};
    bool inProgress{false};
    bool ready{false};
    bool truncated{false};
};

class I2cBusDevice final : public DeviceRuntimeBase {
public:
    class DependencyTransaction {
    public:
        DependencyTransaction() = default;
        DependencyTransaction(I2cBusDevice* bus, II2cBusDriver* driver, uint32_t generation);
        DependencyTransaction(const DependencyTransaction&) = delete;
        DependencyTransaction& operator=(const DependencyTransaction&) = delete;
        DependencyTransaction(DependencyTransaction&& other) noexcept;
        DependencyTransaction& operator=(DependencyTransaction&& other) noexcept;
        ~DependencyTransaction();

        explicit operator bool() const;
        II2cBusDriver* driver() const;
        uint32_t generation() const;
        void release();

    private:
        I2cBusDevice* bus_{nullptr};
        II2cBusDriver* driver_{nullptr};
        uint32_t generation_{0};
    };

    I2cBusDevice(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob);
    I2cBusDevice(const I2cBusDeviceConfigV1& config, II2cBusDriver& driver);

    const I2cBusDeviceConfigV1& config() const;
    bool enabled() const override;
    const char* name() const override;
    uint32_t generation() const;
    bool dependencyTransactionActive() const;
    const I2cScanResult& scan() const;
    const BusRuntimeDiagnostics& diagnostics() const;
    DependencyTransaction beginDependencyTransaction();
    void resetDiagnostics(uint32_t now);
    void recordDiagnosticsError(uint32_t errorCode, uint32_t now);
    void recordDiagnosticsSuccess();
    void bindDeviceIdentity(const DeviceRegistryEntry& record, const DeviceConfigBlob& config) override;
    bool serializeConfigBlob(DeviceConfigBlob& configBlob) const override;
    bool replaceBaseConfig(DeviceConfigBlob& configBlob, const DeviceBaseConfigV1& baseConfig) const override;
    DeviceConfigUpdatePlan planConfigUpdate(const DeviceConfigBlob& configBlob) const override;
    bool applyConfig(const DeviceConfigBlob& configBlob, uint32_t now) override;
    void end(uint32_t now) override;
    void writeDeviceJson(JsonObject output) const;

    static DeviceTypeDescriptor descriptor();
    static std::unique_ptr<IDeviceRuntime> createRuntime(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob);
    static DeviceValidationResult validateConfig(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob);

    bool handleCommand(const DeviceCommand& command) override;
    bool hasDuplicateDependentI2cAddress(uint8_t address, const IDeviceRuntime* ignoreDependent = nullptr) const override;

private:
    I2cBusDevice(const I2cBusDeviceConfigV1& config, std::unique_ptr<II2cBusDriver> ownedDriver);

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
    void startScan();
    void finishScan();
    void cancelScan();
    void appendScanAddress(uint8_t address);
    void releaseHardware();
    void releaseDependencyTransaction();
    DeviceValidationResult initializeHardware(uint32_t now);
    bool isScanning() const;

    I2cBusDeviceConfigV1 config_{};
    std::unique_ptr<II2cBusDriver> ownedDriver_{};
    II2cBusDriver& driver_;
    I2cScanResult scan_{};
    BusRuntimeDiagnostics diagnostics_{};
    uint32_t generation_{0};
    bool dependencyTransactionActive_{false};
};

} // namespace ewfm
