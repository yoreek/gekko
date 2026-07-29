#pragma once

#include "devices/sensors/binary/IGpioInputDriver.h"
#include "wifi/WifiManager.h"

#include <cstdint>

namespace ewfm {

class BleProvisioningButton {
public:
    BleProvisioningButton(IGpioInputDriver& input, WifiManager& wifiManager, uint8_t pin);

    bool begin(uint32_t now);
    void tick(uint32_t now);

    static constexpr uint32_t kDebounceMs = 50;
    static constexpr uint32_t kHoldMs = 3000;

private:
    IGpioInputDriver& input_;
    WifiManager& wifiManager_;
    uint8_t pin_;
    uint32_t rawChangedAt_{0};
    uint32_t pressedAt_{0};
    bool rawPressed_{false};
    bool stablePressed_{false};
    bool activationSent_{false};
    bool configured_{false};
};

} // namespace ewfm
