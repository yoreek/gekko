#pragma once

#include <string>

namespace ewfm {

class ArduinoOtaService final {
public:
    void begin(const std::string& hostname);
    void tick();

    bool started() const {
        return started_;
    }

private:
    std::string hostname_;
    bool started_{false};
};

} // namespace ewfm
