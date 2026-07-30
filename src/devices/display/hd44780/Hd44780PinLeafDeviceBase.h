#pragma once

#include "devices/display/hd44780/Hd44780PinCharacterDisplayDeviceBase.h"

#include <cstring>

namespace ewfm {

inline DeviceTypeDescriptor hd44780PinLeafDeviceDescriptor(DeviceTypeId typeId, const char* name, uint32_t currentConfigVersion) {
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
        {DeviceRole::MetricSource, false},
    };
    return descriptor;
}

// Shared leaf for the direct-GPIO HD44780 displays (lcd1602_pin, lcd2004_pin, ...) -- no
// dependency at all beyond the optional layout-derived MetricSource list, mirroring
// Tm1637Device/Ds1302RtcDevice.
template <typename Derived, typename Config, bool (*DecodeConfig)(const uint8_t*, size_t, Config&), size_t (*ConfigSize)(const Config&)>
class Hd44780PinLeafDeviceBase : public Hd44780PinCharacterDisplayDeviceBase {
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
        plan.endOldConfig = config.rsPin != config_.rsPin || config.ePin != config_.ePin || config.d4Pin != config_.d4Pin ||
                            config.d5Pin != config_.d5Pin || config.d6Pin != config_.d6Pin || config.d7Pin != config_.d7Pin ||
                            config.backlightPin != config_.backlightPin;
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
        resetRenderedLines();
        return true;
    }

protected:
    Hd44780PinLeafDeviceBase(PState initialState, uint8_t columns, uint8_t rows, const Config& config)
        : Hd44780PinCharacterDisplayDeviceBase(initialState, columns, rows), config_(config) {}

    Hd44780PinLeafDeviceBase(PState initialState, uint8_t columns, uint8_t rows, const Config& config, IHd44780PinLineDriver& lineDriver)
        : Hd44780PinCharacterDisplayDeviceBase(initialState, columns, rows, lineDriver), config_(config) {}

    Hd44780PinLeafDeviceBase(PState initialState, uint8_t columns, uint8_t rows, const DeviceRegistryEntry& record,
                             const DeviceConfigBlob& configBlob)
        : Hd44780PinLeafDeviceBase(initialState, columns, rows, decodeConfig(configBlob)) {
        bindDeviceIdentity(record, configBlob);
    }

    const DeviceBaseConfigV1& baseConfig() const override {
        return config_;
    }

    Hd44780PinLineChannels pinLineChannels() const override {
        return {config_.rsPin, config_.ePin, config_.d4Pin, config_.d5Pin, config_.d6Pin, config_.d7Pin, config_.backlightPin};
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
