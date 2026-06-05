#pragma once

#include "wifi/WifiManager.h"

#include <cstdint>
#include <string>

namespace ewfm {

class ArduinoOtaService final {
public:
    void begin(const std::string& hostname, const WifiManager& wifiManager);
    void tick(uint32_t now);

    bool started() const {
        return started_;
    }

private:
    void start();

    const WifiManager* wifiManager_{nullptr};
    std::string hostname_;
    bool configured_{false};
    bool started_{false};
};

} // namespace ewfm
