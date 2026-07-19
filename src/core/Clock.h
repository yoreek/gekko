#pragma once

#include <cstdint>

namespace ewfm {

class IClock {
public:
    virtual ~IClock() = default;
    virtual uint32_t millis() const = 0;
};

class ManualClock final : public IClock {
public:
    uint32_t millis() const override {
        return now_;
    }
    void set(uint32_t now) {
        now_ = now;
    }
    void advance(uint32_t delta) {
        now_ += delta;
    }

private:
    uint32_t now_{0};
};

} // namespace ewfm
