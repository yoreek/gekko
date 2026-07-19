#pragma once

#include "config/ConfigStore.h"
#include "core/StateMachine.h"
#include "time/DateTime.h"
#include "time/NtpClient.h"
#include "wifi/WifiManager.h"

#include <cstdint>
#include <optional>

namespace ewfm {

// Tracks whether the current synced time came from an NTP exchange, a manual REST override, or a
// hardware RTC device (RtcSyncCoordinator), so the status endpoint can report an honest "source".
enum class TimeSource {
    None,
    Ntp,
    Manual,
    Rtc,
};

// "ntp" is the default/fallback label (used for both TimeSource::Ntp and the unsynced
// TimeSource::None case, matching the REST/WS status endpoints' pre-existing behavior).
const char* timeSourceName(TimeSource source);

// Non-blocking NTP time sync, modeled on MqttManager's WiFi-station-gated shape (not
// WifiManager's own-the-hardware-lifecycle shape): WaitingForStation gates on
// WifiManager::stationReady(), and applySettings() bumps a revision counter so a settings change
// (new NTP server / timezone / sync interval) forces an immediate resync instead of waiting out
// the normal periodic interval. TimeConfig.enabled additionally gates readiness - when disabled,
// the state machine just parks in WaitingForStation forever (see enabled()/ntpReady()).
//
// RTC integration: RtcSyncCoordinator (src/time/RtcSyncCoordinator.h) is a peer App service that
// seeds system time from a hardware RTC device early at boot (via seedFromRtc(), well before WiFi
// association completes), writes system time back to the RTC after each authoritative NTP/manual
// sync, and falls back to the RTC again if NTP stays unreachable for a long time. It reads
// hasAuthoritativeSync()/lastAuthoritativeSyncMs()/syncRevision() to drive that policy without
// this class needing to know about the device registry.
class NtpManager final : public StateMachine {
public:
    NtpManager(INtpClient& client, ConfigStore& configStore);

    // Wires the WiFi dependency only; does not start resolving (mirrors MqttManager::begin).
    void begin(const WifiManager& wifiManager);

    // Validates + persists via ConfigStore, then bumps a revision counter so a currently-running
    // resolve/sync cycle restarts with the new settings. Can be called repeatedly at runtime (e.g.
    // from a REST settings update).
    ValidationResult applySettings(const TimeConfig& config);

    // Applies a caller-supplied UTC time immediately (e.g. from a REST manual-set request),
    // regardless of whether NTP sync is enabled - useful when NTP is disabled entirely, or as a
    // one-off correction while waiting for the next scheduled resync. Counts as an authoritative
    // sync (see hasAuthoritativeSync()) and triggers an RTC write-back via RtcSyncCoordinator.
    void setManualTime(uint32_t utcEpoch);

    // Seeds/corrects system time from a hardware RTC device (RtcSyncCoordinator). Unlike
    // setManualTime()/a successful NTP sync, this does NOT count as an authoritative sync - it
    // exists so NTP keeps retrying independently and the status endpoint can honestly report
    // TimeSource::Rtc as a weaker, stand-in source rather than as good as a real sync.
    void seedFromRtc(uint32_t utcEpoch) {
        applyExternalTime(utcEpoch, TimeSource::Rtc);
    }

    [[nodiscard]] bool synced() const {
        return synced_;
    }
    [[nodiscard]] const std::optional<DateTime>& lastSyncedUtc() const {
        return lastSyncedUtc_;
    }
    [[nodiscard]] TimeSource lastSource() const {
        return lastSource_;
    }
    // True once NTP or a manual REST call has set the time at least once (RTC-only seeds don't
    // count). RtcSyncCoordinator uses this to decide whether a boot-time RTC seed is still needed.
    [[nodiscard]] bool hasAuthoritativeSync() const {
        return hasAuthoritativeSync_;
    }
    // Uptime (ms, from the StateMachine tick clock) of the last authoritative (Ntp/Manual) sync.
    // Only meaningful once hasAuthoritativeSync() is true.
    [[nodiscard]] uint32_t lastAuthoritativeSyncMs() const {
        return lastAuthoritativeSyncMs_;
    }
    // Bumped on every authoritative (Ntp/Manual) sync. RtcSyncCoordinator compares this against
    // its own last-observed value to detect "a fresh sync just happened" and trigger an RTC
    // write-back, without NtpManager needing to know about the device registry.
    [[nodiscard]] uint32_t syncRevision() const {
        return syncRevision_;
    }
    [[nodiscard]] const TimeConfig& config() const {
        return configStore_.config().time;
    }
    [[nodiscard]] bool enabled() const {
        return configStore_.config().time.enabled;
    }
    [[nodiscard]] bool waitingForStation() const {
        return is((PState)&NtpManager::WaitingForStation);
    }

private:
    void Idle();
    void WaitingForStation();
    void ResolveServer();
    void Syncing();
    void CheckSynced();
    void Synced();
    void RetryDelay();

    [[nodiscard]] bool ntpReady() const;
    void applyExternalTime(uint32_t utcEpoch, TimeSource source);

    static constexpr uint32_t kResolveTimeoutMs = 30000;
    static constexpr uint32_t kResponseTimeoutMs = 3000;
    static constexpr uint32_t kRetryDelayMs = 60000; // flat retry, no backoff (matches the ReefDuino reference)

    INtpClient& client_;
    ConfigStore& configStore_;
    const WifiManager* wifiManager_{nullptr};
    uint32_t settingsRevision_{0};
    uint32_t appliedRevision_{0};
    bool configured_{false};
    bool synced_{false};
    bool stationWaitLogged_{false};
    std::optional<DateTime> lastSyncedUtc_{};
    TimeSource lastSource_{TimeSource::None};
    bool hasAuthoritativeSync_{false};
    uint32_t lastAuthoritativeSyncMs_{0};
    uint32_t syncRevision_{0};
};

} // namespace ewfm
