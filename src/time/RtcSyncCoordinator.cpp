#include "time/RtcSyncCoordinator.h"

#include "debug/Debug.h"

namespace ewfm {

namespace {
bool elapsedSince(uint32_t now, uint32_t reference, uint32_t intervalMs) {
    return static_cast<uint32_t>(now - reference) > intervalMs;
}
} // namespace

RtcSyncCoordinator::RtcSyncCoordinator(NtpManager& ntpManager, DeviceRegistry& deviceRegistry)
    : ntpManager_(ntpManager), deviceRegistry_(deviceRegistry) {}

const IRealTimeClockRuntime* RtcSyncCoordinator::findActiveRtc() const {
    const IRealTimeClockRuntime* found = nullptr;
    deviceRegistry_.forEachRuntime([&](const IDeviceRuntime& runtime) {
        if (found != nullptr || runtime.status() != DeviceStatus::Ready) {
            return;
        }
        const IRealTimeClockRuntime* rtc = runtime.realTimeClockRuntime();
        if (rtc != nullptr && rtc->useForSystemTimeSync()) {
            found = rtc;
        }
    });
    return found;
}

void RtcSyncCoordinator::tick(uint32_t now) {
    if (!elapsedSince(now, lastTickMs_, kTickIntervalMs)) {
        return;
    }
    lastTickMs_ = now;

    const IRealTimeClockRuntime* activeRtc = findActiveRtc();
    tickBootSeed(now, activeRtc);
    tickWriteBack(now, activeRtc);
    tickStalenessFallback(now, activeRtc);
}

void RtcSyncCoordinator::tickBootSeed(uint32_t now, const IRealTimeClockRuntime* activeRtc) {
    if (ntpManager_.hasAuthoritativeSync() || activeRtc == nullptr) {
        return;
    }
    if (attemptedBootSeed_ && !elapsedSince(now, lastBootSeedAttemptMs_, kBootReseedRetryMs)) {
        return;
    }

    attemptedBootSeed_ = true;
    lastBootSeedAttemptMs_ = now;

    uint32_t epoch = 0;
    bool lostPower = false;
    if (!activeRtc->latestTimeReading(epoch, lostPower)) {
        return;
    }
    if (lostPower) {
        EWFM_RTC_SYNC_LOG_WARN("rtc reports lost power - boot seed epoch may be inaccurate");
    }
    ntpManager_.seedFromRtc(epoch);
    EWFM_RTC_SYNC_LOG_INFO("system time seeded from rtc epoch=%lu", static_cast<unsigned long>(epoch));
}

void RtcSyncCoordinator::tickWriteBack(uint32_t now, const IRealTimeClockRuntime* activeRtc) {
    const bool freshSync = ntpManager_.syncRevision() != observedSyncRevision_;
    const bool periodicDue = ntpManager_.hasAuthoritativeSync() && elapsedSince(now, lastWriteBackMs_, kWriteBackIntervalMs);
    if (!freshSync && !periodicDue) {
        return;
    }

    // Only mark the fresh-sync signal as consumed once we've actually confirmed a write attempt is
    // possible. Consuming it earlier (e.g. while activeRtc is still nullptr because the RTC device
    // hasn't reached Ready yet) would silently burn the opportunity: the next chance to write back
    // would then be gated behind kWriteBackIntervalMs instead of retrying on the next tick.
    if (activeRtc == nullptr || !ntpManager_.hasAuthoritativeSync()) {
        return;
    }
    const std::optional<DateTime>& lastSynced = ntpManager_.lastSyncedUtc();
    if (!lastSynced.has_value()) {
        return;
    }

    observedSyncRevision_ = ntpManager_.syncRevision();
    lastWriteBackMs_ = now;

    // const_cast is safe here: writeTime() is a mutating hardware operation on the RTC device,
    // but findActiveRtc() only hands out const access because most callers only ever read.
    if (const_cast<IRealTimeClockRuntime*>(activeRtc)->writeTime(lastSynced->utcUnixtime())) {
        EWFM_RTC_SYNC_LOG_INFO("system time written back to rtc epoch=%lu", static_cast<unsigned long>(lastSynced->utcUnixtime()));
    }
}

void RtcSyncCoordinator::tickStalenessFallback(uint32_t now, const IRealTimeClockRuntime* activeRtc) {
    if (!ntpManager_.hasAuthoritativeSync() || activeRtc == nullptr) {
        return;
    }
    if (!elapsedSince(now, ntpManager_.lastAuthoritativeSyncMs(), kStalenessThresholdMs)) {
        return;
    }
    if (!elapsedSince(now, lastStalenessSeedMs_, kStalenessReseedIntervalMs)) {
        return;
    }

    lastStalenessSeedMs_ = now;
    uint32_t epoch = 0;
    bool lostPower = false;
    if (!activeRtc->latestTimeReading(epoch, lostPower)) {
        return;
    }
    ntpManager_.seedFromRtc(epoch);
    EWFM_RTC_SYNC_LOG_WARN("ntp sync stale, corrected system time from rtc epoch=%lu", static_cast<unsigned long>(epoch));
}

} // namespace ewfm
