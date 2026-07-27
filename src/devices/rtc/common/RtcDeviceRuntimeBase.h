#pragma once

#include "devices/core/DeviceTypes.h"
#include "devices/rtc/common/RtcReadingState.h"

#include <cstdint>
#include <utility>

namespace ewfm {

// Non-owning bookkeeping shared by every IRealTimeClockRuntime device (DS3231, DS1302, ...).
// Deliberately owns none of the state machine - DS3231's dependency-bus/busy-retry/generation-
// tracking states have no DS1302 equivalent (bit-banged, zero dependencies), so forcing them into
// one owning base would either dead-code half of it for DS1302 or hide most of DS3231's actual
// behavior behind hooks only DS3231 implements. See docs/device-scaffolding.md's "size a base to
// one read model" note (ds18b20/htu21 stayed separate from PolledTemperatureSensorDeviceBase for
// the same reason). Templated over RuntimeBase, mirroring I2cDeviceRuntimeBase<Derived, RuntimeBase>,
// so it can sit either directly on DeviceRuntimeBase (DS1302) or on top of the I2C bus-dependency
// template chain (DS3231) without a diamond.
template <typename Derived, typename RuntimeBase> class RtcDeviceRuntimeBase : public RuntimeBase, public IRealTimeClockRuntime {
protected:
    template <typename... Args> explicit RtcDeviceRuntimeBase(Args&&... args) : RuntimeBase(std::forward<Args>(args)...) {}

    const DeviceBaseConfigV1& baseConfig() const override {
        return static_cast<const Derived*>(this)->config();
    }

    // Returns true if the caller should mark runtime state dirty (already done here).
    bool recordSuccess(uint32_t epoch, bool lostPower) {
        if (readingState_.recordSuccess(epoch, lostPower)) {
            this->markRuntimeStateDirty();
            return true;
        }
        return false;
    }

    // Deliberately does not compute a retry deadline - Faulted, and RetryBackoff for devices with a
    // single entry path (DS1302), always wait a fixed duration relative to when that state was
    // entered, so the caller's state body can use StateMachine::elapsed(uptime(), kFixedMs) instead
    // of tracking an absolute deadline field. DS3231's RetryBackoff is entered via two paths with
    // different durations (bus-busy vs read-failure) and keeps its own retryDeadline_ locally.
    bool recordFailure() {
        if (readingState_.recordFailure()) {
            this->markRuntimeStateDirty();
            return true;
        }
        return false;
    }

    static constexpr uint32_t kPollMs = 1000;
    static constexpr uint32_t kRetryBackoffMs = 1000;
    static constexpr uint32_t kFaultRetryBackoffMs = 30000;
    static constexpr uint8_t kFaultErrorThreshold = 3;

    RtcReadingState readingState_;

public:
    const IRealTimeClockRuntime* realTimeClockRuntime() const override {
        return this;
    }
    bool latestTimeReading(uint32_t& outUtcEpoch, bool& outLostPower) const override {
        return readingState_.latestTimeReading(outUtcEpoch, outLostPower);
    }
    bool useForSystemTimeSync() const override {
        return static_cast<const Derived*>(this)->config().useForSystemTimeSync != 0U;
    }
    bool lastReadOk() const {
        return readingState_.lastReadOk();
    }
};

} // namespace ewfm
