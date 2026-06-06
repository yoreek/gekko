#include "platform/ArduinoOtaService.h"

#include "debug/Debug.h"

#if defined(ARDUINO) && !defined(UNIT_TEST) && defined(WITH_ARDUINO_OTA)
#include <ArduinoOTA.h>

namespace {
void onOtaStart() {
    EWFM_APP_LOG_INFO("dev OTA started");
}

void onOtaEnd() {
    EWFM_APP_LOG_INFO("dev OTA finished");
}

void onOtaProgress(unsigned int progress, unsigned int total) {
    EWFM_LOG_DEBUG("app", "dev OTA progress=%u/%u", progress, total);
}

void onOtaError(ota_error_t error) {
    EWFM_LOG_WARN("app", "dev OTA error=%u", static_cast<unsigned>(error));
}
} // namespace
#endif

namespace ewfm {

#undef SM_CLASS
#define SM_CLASS ArduinoOtaService

ArduinoOtaService::ArduinoOtaService() : StateMachine((PState)&ArduinoOtaService::Idle) {}

void ArduinoOtaService::begin(const std::string& hostname, const WifiManager& wifiManager) {
    if (started_) {
        return;
    }

    hostname_ = hostname;
    wifiManager_ = &wifiManager;
    configured_ = true;
}

bool ArduinoOtaService::waitingForStation() const {
    return is((PState)&ArduinoOtaService::WaitingForStation);
}

bool ArduinoOtaService::running() const {
    return is((PState)&ArduinoOtaService::Running);
}

SM_STATE(Idle) {
    if (!configured_ || wifiManager_ == nullptr) {
        return;
    }
    SM_GOTO(WaitingForStation);
}

SM_STATE(WaitingForStation) {
    if (!configured_ || wifiManager_ == nullptr) {
        SM_GOTO(Idle);
    }

    if (!otaReady()) {
        if (!stationWaitLogged_) {
            EWFM_APP_LOG_INFO("dev OTA waiting for WiFi/IP readiness");
            stationWaitLogged_ = true;
        }
        return;
    }

    stationWaitLogged_ = false;
    SM_GOTO(Starting);
}

SM_STATE(Starting) {
    if (!otaReady()) {
        SM_GOTO(WaitingForStation);
    }

    start();
    if (!started_) {
        SM_GOTO(Faulted);
    }
    SM_GOTO(Running);
}

SM_STATE(Running) {
    if (!otaReady() || otaIpChanged()) {
        stop();
        SM_GOTO(WaitingForStation);
    }

#if defined(ARDUINO) && !defined(UNIT_TEST) && defined(WITH_ARDUINO_OTA)
    ArduinoOTA.handle();
#endif
}

SM_STATE(Faulted) {
    if (!otaReady()) {
        SM_GOTO(WaitingForStation);
    }

    EWFM_APP_LOG_INFO("dev OTA faulted, retrying start");
    SM_GOTO(Starting);
}

void ArduinoOtaService::start() {
    if (started_) {
        return;
    }

#if defined(ARDUINO) && !defined(UNIT_TEST) && defined(WITH_ARDUINO_OTA)
    ArduinoOTA.setHostname(hostname_.c_str());
    ArduinoOTA.onStart(onOtaStart);
    ArduinoOTA.onEnd(onOtaEnd);
    ArduinoOTA.onProgress(onOtaProgress);
    ArduinoOTA.onError(onOtaError);
    ArduinoOTA.begin();
#endif
    started_ = true;
    activeIp_ = wifiManager_ != nullptr ? wifiManager_->otaIp() : std::string{};
    ++startCount_;
}

void ArduinoOtaService::stop() {
    if (!started_) {
        activeIp_.clear();
        return;
    }

#if defined(ARDUINO) && !defined(UNIT_TEST) && defined(WITH_ARDUINO_OTA)
    ArduinoOTA.end();
#endif
    started_ = false;
    activeIp_.clear();
    ++stopCount_;
}

bool ArduinoOtaService::otaReady() const {
    return wifiManager_ != nullptr && wifiManager_->otaReady();
}

bool ArduinoOtaService::otaIpChanged() const {
    return wifiManager_ != nullptr && !activeIp_.empty() && wifiManager_->otaIp() != activeIp_;
}

} // namespace ewfm
