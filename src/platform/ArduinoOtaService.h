#pragma once

#include "core/StateMachine.h"
#include "wifi/WifiManager.h"

#include <cstdint>
#include <string>

namespace ewfm {

class ArduinoOtaService final : public StateMachine {
public:
    ArduinoOtaService();

    void begin(const std::string& hostname, const WifiManager& wifiManager);

    bool started() const {
        return started_;
    }
    bool waitingForStation() const;
    bool running() const;
#if defined(UNIT_TEST)
    uint16_t startCount() const {
        return startCount_;
    }
    uint16_t stopCount() const {
        return stopCount_;
    }
#endif

private:
    void Idle();
    void WaitingForStation();
    void Starting();
    void Running();
    void Faulted();
    void start();
    void stop();
    bool otaReady() const;
    bool otaIpChanged() const;

    const WifiManager* wifiManager_{nullptr};
    std::string hostname_;
    std::string activeIp_;
    bool stationWaitLogged_{false};
    bool configured_{false};
    bool started_{false};
    uint16_t startCount_{0};
    uint16_t stopCount_{0};
};

} // namespace ewfm
