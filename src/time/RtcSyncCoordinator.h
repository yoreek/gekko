#pragma once

#include "devices/registry/DeviceRegistry.h"
#include "time/NtpManager.h"

#include <cstdint>

namespace ewfm {

// Bridges the single, registry-wide NtpManager (system time) with whichever DeviceRegistry entry
// currently provides DeviceRole::RealTimeClock and has opted in via useForSystemTimeSync(). REST
// validation (Ds3231RtcDeviceApiAdapter) enforces that at most one device has that flag set, so
// findActiveRtc() never has to arbitrate between candidates.
//
// Policy (see NtpManager::seedFromRtc/hasAuthoritativeSync/syncRevision/lastAuthoritativeSyncMs):
//  1. Boot/outage seed: while NtpManager has never had an authoritative (Ntp/Manual) sync, seed
//     system time from the RTC, retried every kBootReseedRetryMs in case the first read failed.
//  2. Write-back: whenever a fresh authoritative sync happens (syncRevision() changes) or
//     kWriteBackIntervalMs has elapsed since the last write-back, push system time to the RTC.
//  3. Staleness fallback: if authoritative sync is more than kStalenessThresholdMs old (NTP has
//     been failing for a long time), periodically re-seed system time from the RTC so the
//     internal software clock does not drift unchecked - without resetting NtpManager's
//     authoritative-sync clock, so NTP keeps retrying independently.
// Driven from the app's ~1s device cadence (see App::tickDeviceCadence), not the raw per-loop
// rate. Even at 1s that's still more often than needed - the policy below only ever acts on
// minutes-to-hours timescales - so tick() additionally self-throttles to kTickIntervalMs before
// re-scanning the device registry (which takes a mutex) or touching the RTC over I2C.
class RtcSyncCoordinator {
public:
    RtcSyncCoordinator(NtpManager& ntpManager, DeviceRegistry& deviceRegistry);

    void tick(uint32_t now);

private:
    static constexpr uint32_t kTickIntervalMs = 30UL * 1000UL;                    // 30 seconds
    static constexpr uint32_t kBootReseedRetryMs = 5UL * 60UL * 1000UL;           // 5 minutes
    static constexpr uint32_t kWriteBackIntervalMs = 60UL * 60UL * 1000UL;        // 1 hour
    static constexpr uint32_t kStalenessThresholdMs = 6UL * 60UL * 60UL * 1000UL; // 6 hours
    static constexpr uint32_t kStalenessReseedIntervalMs = 30UL * 60UL * 1000UL;  // 30 minutes

    const IRealTimeClockRuntime* findActiveRtc() const;
    void tickBootSeed(uint32_t now, const IRealTimeClockRuntime* activeRtc);
    void tickWriteBack(uint32_t now, const IRealTimeClockRuntime* activeRtc);
    void tickStalenessFallback(uint32_t now, const IRealTimeClockRuntime* activeRtc);

    NtpManager& ntpManager_;
    DeviceRegistry& deviceRegistry_;
    uint32_t lastTickMs_{0};
    bool attemptedBootSeed_{false};
    uint32_t lastBootSeedAttemptMs_{0};
    uint32_t lastWriteBackMs_{0};
    uint32_t lastStalenessSeedMs_{0};
    uint32_t observedSyncRevision_{0};
};

} // namespace ewfm
