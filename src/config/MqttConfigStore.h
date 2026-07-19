#pragma once

#include "config/ConfigStore.h"
#include "config/MqttConfig.h"

#include <cstdint>
#include <vector>

namespace ewfm {

class MqttConfigStore {
public:
    explicit MqttConfigStore(IConfigStorage& storage) : storage_(storage) {}

    bool begin();
    const MqttSettings& settings() const {
        return settings_;
    }

    MqttConfigValidationResult load();
    MqttConfigValidationResult save(const MqttSettings& settings);

    bool hasCaCert() const;
    MqttConfigValidationResult loadCaCert(std::vector<uint8_t>& pem) const;
    MqttConfigValidationResult saveCaCert(const uint8_t* data, size_t size);
    bool clearCaCert();

private:
    IConfigStorage& storage_;
    MqttSettings settings_{};
};

} // namespace ewfm
