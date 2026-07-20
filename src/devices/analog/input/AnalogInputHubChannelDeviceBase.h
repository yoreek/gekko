#pragma once

#include "devices/analog/input/AdcSampleAccumulator.h"
#include "devices/analog/input/AnalogInputReadingPublisher.h"
#include "devices/core/DeviceRuntimeBase.h"

namespace ewfm {

// Shared behavior for AnalogInputChannelDevice: poll one channel of whatever hub is wired up via
// the generic DeviceRole::AnalogInputHub dependency (accessed only through
// IAnalogInputHubRuntime, so this class is hub-implementation-agnostic -- it works unchanged with
// an ADS1115, a CD74HC4067, or any future hub that implements that role, mirroring how
// PortExpanderSwitchDevice is expander-implementation-agnostic). There is exactly one concrete
// leaf, not one per hub chip, precisely because this base is already fully hub-agnostic.
//
// The hub arbitrates one (channel, requester) request at a time and never blocks
// (IAnalogInputHubRuntime::pollChannelReading is non-blocking); this base drives that protocol
// tick by tick: request a sample, stay in the same state across Busy/Pending polls, accumulate
// each Ready sample, and repeat until `adcSampleCount()` samples have been averaged.
class AnalogInputHubChannelDeviceBase : public DeviceRuntimeBase, public IAnalogInputRuntime {
public:
    const AnalogInputReading& reading() const;
    const char* outputStatus() const;
    bool latestAnalogInputReading(AnalogInputReading& reading) const override;
    const char* latestAnalogInputStatus() const override;
    const IAnalogInputRuntime* analogInputRuntime() const override;
    bool expanderChannel(uint8_t& channel) const override;
    void end(uint32_t now) override;

protected:
    explicit AnalogInputHubChannelDeviceBase(PState initialState);

    // Hooks the concrete leaf supplies from its own config struct.
    virtual uint8_t channel() const = 0;
    virtual uint8_t adcSampleCount() const = 0;
    virtual bool reportAlways() const = 0;
    virtual uint16_t reportDeltaMilliVolts() const = 0;
    virtual uint32_t pollIntervalMs() const = 0;
    virtual bool channelEnabled() const = 0;

    State Idle();
    State Starting();
    State Sampling();
    State Ready();
    State RetryBackoff();
    State DependencyBlocked();
    State Reconfiguring();
    State Disabled();
    State Faulted();
    State Deleting();

private:
    static constexpr uint32_t kRetryBackoffMs = 1000;
    static constexpr uint32_t kFaultRetryBackoffMs = 30000;
    static constexpr uint8_t kFaultErrorThreshold = 3;

    IAnalogInputHubRuntime* dependencyHub() const;
    bool dependencyHubReady() const;
    bool dependencyHubGenerationChanged() const;
    void releaseHubClaim();
    void recordFailure(uint32_t now);
    void invalidateAndMark(const char* status);

    AdcSampleAccumulator accumulator_{};
    AnalogInputReadingPublisher publisher_{};
    uint32_t nextPollAt_{0};
    uint32_t retryDeadline_{0};
    uint32_t lastHubGeneration_{0};
    uint8_t consecutiveErrors_{0};
};

} // namespace ewfm
