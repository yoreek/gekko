#include "time/NtpManager.h"

#include "debug/Debug.h"

#if defined(ARDUINO) && !defined(UNIT_TEST)
#include <TimeLib.h>
#endif

namespace ewfm {

const char* timeSourceName(TimeSource source) {
    switch (source) {
    case TimeSource::Manual:
        return "manual";
    case TimeSource::Rtc:
        return "rtc";
    case TimeSource::Ntp:
    case TimeSource::None:
    default:
        return "ntp";
    }
}

#undef SM_CLASS
#define SM_CLASS NtpManager

NtpManager::NtpManager(INtpClient& client, ConfigStore& configStore)
    : StateMachine((PState)&NtpManager::Idle), client_(client), configStore_(configStore) {}

void NtpManager::begin(const WifiManager& wifiManager) {
    wifiManager_ = &wifiManager;
    configured_ = true;
}

ValidationResult NtpManager::applySettings(const TimeConfig& config) {
    const ValidationResult result = configStore_.saveTimeConfig(config);
    if (!result.ok()) {
        EWFM_NTP_LOG_WARN("ntp settings rejected: %s", result.message);
        return result;
    }
    ++settingsRevision_;
    return result;
}

void NtpManager::setManualTime(uint32_t utcEpoch) {
    applyExternalTime(utcEpoch, TimeSource::Manual);
    EWFM_NTP_LOG_INFO("time set manually epoch=%lu", static_cast<unsigned long>(utcEpoch));
}

void NtpManager::applyExternalTime(uint32_t utcEpoch, TimeSource source) {
    synced_ = true;
    lastSyncedUtc_ = DateTime(utcEpoch);
    lastSource_ = source;
#if defined(ARDUINO) && !defined(UNIT_TEST)
    setTime(utcEpoch);
#endif
    if (source != TimeSource::Rtc) {
        hasAuthoritativeSync_ = true;
        lastAuthoritativeSyncMs_ = uptime();
        ++syncRevision_;
    }
}

bool NtpManager::ntpReady() const {
    return configured_ && enabled() && wifiManager_ != nullptr && wifiManager_->stationReady();
}

SM_STATE(Idle) {
    if (!configured_) {
        return;
    }
    SM_GOTO(WaitingForStation);
}

SM_STATE(WaitingForStation) {
    if (!configured_) {
        SM_GOTO(Idle);
    }

    if (!ntpReady()) {
        if (!stationWaitLogged_) {
            EWFM_NTP_LOG_INFO("waiting for wifi station readiness");
            stationWaitLogged_ = true;
        }
        return;
    }

    stationWaitLogged_ = false;
    appliedRevision_ = settingsRevision_;
    SM_GOTO(ResolveServer);
}

SM_STATE(ResolveServer) {
    if (!ntpReady()) {
        SM_GOTO(WaitingForStation);
    }

    if (isStateUpdated()) {
        client_.beginResolve(configStore_.config().time.ntpServer);
    }

    const NtpResolveStatus status = client_.resolveStatus();
    if (status == NtpResolveStatus::Resolved) {
        SM_GOTO(Syncing);
    }
    if (status == NtpResolveStatus::Failed || isTimeout(kResolveTimeoutMs)) {
        EWFM_NTP_LOG_WARN("ntp server resolve failed or timed out");
        SM_GOTO(RetryDelay);
    }
}

SM_STATE(Syncing) {
    if (!ntpReady()) {
        SM_GOTO(WaitingForStation);
    }

    if (isStateUpdated()) {
        if (!client_.openSocket() || !client_.sendRequest()) {
            EWFM_NTP_LOG_WARN("ntp request send failed");
            SM_GOTO(RetryDelay);
        }
    }
    SM_GOTO(CheckSynced);
}

SM_STATE(CheckSynced) {
    if (!ntpReady()) {
        client_.closeSocket();
        SM_GOTO(WaitingForStation);
    }

    uint32_t epoch = 0;
    if (client_.pollResponse(epoch)) {
        client_.closeSocket();
        applyExternalTime(epoch, TimeSource::Ntp);
        EWFM_NTP_LOG_INFO("ntp sync succeeded epoch=%lu", static_cast<unsigned long>(epoch));
        SM_GOTO(Synced);
    }
    if (isTimeout(kResponseTimeoutMs)) {
        client_.closeSocket();
        EWFM_NTP_LOG_WARN("ntp response timed out");
        SM_GOTO(RetryDelay);
    }
}

SM_STATE(Synced) {
    if (!ntpReady()) {
        SM_GOTO(WaitingForStation);
    }
    if (appliedRevision_ != settingsRevision_) {
        appliedRevision_ = settingsRevision_;
        EWFM_NTP_LOG_INFO("ntp settings changed, resyncing");
        SM_GOTO(ResolveServer);
    }
    if (isTimeout(configStore_.config().time.syncIntervalSeconds * 1000UL)) {
        SM_GOTO(ResolveServer);
    }
}

SM_STATE(RetryDelay) {
    if (!ntpReady()) {
        SM_GOTO(WaitingForStation);
    }
    if (isTimeout(kRetryDelayMs)) {
        SM_GOTO(WaitingForStation);
    }
}

} // namespace ewfm
