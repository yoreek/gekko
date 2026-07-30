#pragma once

#include "devices/bus/i2c/I2cDeviceRuntimeBase.h"
#include "devices/display/hd44780/Hd44780I2cCharacterDisplayDeviceBase.h"

#include <cstring>

namespace ewfm {

inline DeviceTypeDescriptor hd44780LeafDeviceDescriptor(DeviceTypeId typeId, const char* name, uint32_t currentConfigVersion) {
    DeviceTypeDescriptor descriptor;
    descriptor.typeId = typeId;
    descriptor.name = name;
    descriptor.currentConfigVersion = currentConfigVersion;
    descriptor.maxDependents = 0;
    descriptor.supportsCommands = false;
    descriptor.supportsRetainedState = false;
    descriptor.defaultPersistencePolicy = DevicePersistencePolicy::Delayed;
    descriptor.ticks100ms = true;
    descriptor.dependencyRequirements = {
        {DeviceRole::I2CBus, true},
        {DeviceRole::MetricSource, false},
    };
    return descriptor;
}

// Shared leaf for the I2C/PCF8574-backed HD44780 displays (lcd1602, lcd2004, ...). Derived also
// picks up i2cAddress()/probeI2cPresence()/validateI2cConfig() from I2cDeviceRuntimeBase, the same
// mixin ssd1306/ads1115/aht10/htu21/ds3231 use for their own I2C dependency.
template <typename Derived, typename Config, bool (*DecodeConfig)(const uint8_t*, size_t, Config&), size_t (*ConfigSize)(const Config&)>
class Hd44780LeafDeviceBase : public I2cDeviceRuntimeBase<Derived, Hd44780I2cCharacterDisplayDeviceBase> {
    using Base = I2cDeviceRuntimeBase<Derived, Hd44780I2cCharacterDisplayDeviceBase>;

public:
    const Config& config() const {
        return config_;
    }

    bool serializeConfigBlob(DeviceConfigBlob& configBlob) const override {
        uint8_t buffer[kMaxDeviceConfigBytes]{};
        const size_t size = ConfigSize(config_);
        return encodeFixedConfigBlob(Config::kMagic, config_, buffer, size) && configBlob.assign(buffer, size);
    }

    DeviceConfigUpdatePlan planConfigUpdate(const DeviceConfigBlob& configBlob) const override {
        Config config{};
        if (!DecodeConfig(configBlob.data(), configBlob.size(), config)) {
            return {};
        }

        DeviceConfigUpdatePlan plan{};
        plan.endOldConfig = config.i2cAddress != config_.i2cAddress || config.rsChannel != config_.rsChannel ||
                            config.eChannel != config_.eChannel || config.d4Channel != config_.d4Channel ||
                            config.d5Channel != config_.d5Channel || config.d6Channel != config_.d6Channel ||
                            config.d7Channel != config_.d7Channel || config.backlightChannel != config_.backlightChannel;
        plan.resetStateMachine = plan.endOldConfig;
        return plan;
    }

    bool applyConfig(const DeviceConfigBlob& configBlob, uint32_t now) override {
        (void)now;
        Config config{};
        if (!DecodeConfig(configBlob.data(), configBlob.size(), config)) {
            return false;
        }
        config_ = config;
        this->resetRenderedLines();
        return true;
    }

protected:
    Hd44780LeafDeviceBase(Hd44780CharacterDisplayDeviceBase::PState initialState, uint8_t columns, uint8_t rows, const Config& config)
        : Base(initialState, columns, rows), config_(config) {}

    Hd44780LeafDeviceBase(Hd44780CharacterDisplayDeviceBase::PState initialState, uint8_t columns, uint8_t rows,
                          const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob)
        : Hd44780LeafDeviceBase(initialState, columns, rows, decodeConfig(configBlob)) {
        this->bindDeviceIdentity(record, configBlob);
    }

    const DeviceBaseConfigV1& baseConfig() const override {
        return config_;
    }

    Hd44780I2cLineChannels i2cLineChannels() const override {
        return {config_.i2cAddress, config_.rsChannel, config_.eChannel,  config_.d4Channel,
                config_.d5Channel,  config_.d6Channel, config_.d7Channel, config_.backlightChannel};
    }

    void writeDisplayConfigJson(JsonObject output) const override {
        config_.writeJson(output);
    }

    Config config_{};

private:
    static Config decodeConfig(const DeviceConfigBlob& configBlob) {
        Config config{};
        (void)DecodeConfig(configBlob.data(), configBlob.size(), config);
        return config;
    }
};

} // namespace ewfm
