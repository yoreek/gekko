#include "wifi/BleProvisioningButton.h"

#if defined(WITH_BLE_PROVISIONING)

#include "debug/Debug.h"

namespace ewfm {

BleProvisioningButton::BleProvisioningButton(IGpioInputDriver& input, WifiManager& wifiManager, const uint8_t pin)
    : input_(input), wifiManager_(wifiManager), pin_(pin) {}

bool BleProvisioningButton::begin(const uint32_t now) {
    configured_ = input_.configureInput(pin_, GpioInputPullMode::PullUp);
    if (!configured_) {
        EWFM_PROV_LOG_WARN("BLE activation button setup failed pin=%u", pin_);
        return false;
    }

    bool level = true;
    if (!input_.read(pin_, level)) {
        configured_ = false;
        input_.release(pin_);
        EWFM_PROV_LOG_WARN("BLE activation button read failed pin=%u", pin_);
        return false;
    }

    rawPressed_ = !level;
    stablePressed_ = rawPressed_;
    rawChangedAt_ = now;
    pressedAt_ = now;
    activationSent_ = false;
    EWFM_PROV_LOG_INFO("BLE activation button ready pin=%u hold=%lu ms", pin_, static_cast<unsigned long>(kHoldMs));
    return true;
}

void BleProvisioningButton::tick(const uint32_t now) {
    if (!configured_) {
        return;
    }

    bool level = true;
    if (!input_.read(pin_, level)) {
        return;
    }

    const bool pressed = !level;
    if (pressed != rawPressed_) {
        rawPressed_ = pressed;
        rawChangedAt_ = now;
    }

    if (stablePressed_ != rawPressed_ && static_cast<uint32_t>(now - rawChangedAt_) >= kDebounceMs) {
        stablePressed_ = rawPressed_;
        activationSent_ = false;
        if (stablePressed_) {
            pressedAt_ = now;
        }
    }

    if (!stablePressed_ || activationSent_ || static_cast<uint32_t>(now - pressedAt_) < kHoldMs) {
        return;
    }

    activationSent_ = true;
    if (wifiManager_.requestBleConfig()) {
        EWFM_PROV_LOG_INFO("BLE config requested by activation button pin=%u", pin_);
    } else {
        EWFM_PROV_LOG_WARN("BLE config button request rejected pin=%u", pin_);
    }
}

} // namespace ewfm

#endif
