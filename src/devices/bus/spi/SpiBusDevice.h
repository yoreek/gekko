#pragma once

#include "devices/bus/spi/ISpiBusDriver.h"
#include "devices/bus/spi/SpiBusConfig.h"
#include "devices/core/DeviceRuntimeBase.h"

#include <ArduinoJson.h>

namespace ewfm {

class SpiBusDevice final : public DeviceRuntimeBase {
public:
    class DependencyTransaction {
    public:
        DependencyTransaction() = default;
        DependencyTransaction(SpiBusDevice* bus, ISpiBusDriver* driver, uint32_t generation);
        DependencyTransaction(const DependencyTransaction&) = delete;
        DependencyTransaction& operator=(const DependencyTransaction&) = delete;
        DependencyTransaction(DependencyTransaction&& other) noexcept;
        DependencyTransaction& operator=(DependencyTransaction&& other) noexcept;
        ~DependencyTransaction();

        explicit operator bool() const;
        ISpiBusDriver* driver() const;
        uint32_t generation() const;
        void release();

    private:
        SpiBusDevice* bus_{nullptr};
        ISpiBusDriver* driver_{nullptr};
        uint32_t generation_{0};
    };

    SpiBusDevice(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob);
    SpiBusDevice(const SpiBusDeviceConfigV1& config, ISpiBusDriver& driver);

    const SpiBusDeviceConfigV1& config() const;
    bool enabled() const override;
    const char* name() const override;
    uint32_t generation() const;
    bool dependencyTransactionActive() const;
    DependencyTransaction beginDependencyTransaction();
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
    bool hasDuplicateDependentSpiChipSelect(uint8_t pin, const IDeviceRuntime* ignoreDependent = nullptr) const override;

private:
    SpiBusDevice(const SpiBusDeviceConfigV1& config, std::unique_ptr<ISpiBusDriver> ownedDriver);

    State Idle();
    State Starting();
    State Ready();
    State Reconfiguring();
    State Disabled();
    State Faulted();
    State Deleting();
    State DependencyBlocked();

    void releaseHardware();
    void releaseDependencyTransaction();
    DeviceValidationResult initializeHardware(uint32_t now);

    SpiBusDeviceConfigV1 config_{};
    std::unique_ptr<ISpiBusDriver> ownedDriver_{};
    ISpiBusDriver& driver_;
    uint32_t generation_{0};
    bool dependencyTransactionActive_{false};
};

} // namespace ewfm
