#include "platform/ArduinoOtaService.h"

#if defined(ARDUINO) && !defined(UNIT_TEST) && defined(WITH_ARDUINO_OTA)
#include "debug/Debug.h"

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

void ArduinoOtaService::begin(const std::string& hostname, const WifiManager& wifiManager) {
    if (started_) {
        return;
    }

    hostname_ = hostname;
    wifiManager_ = &wifiManager;
    configured_ = true;
}

void ArduinoOtaService::tick(uint32_t now) {
    (void)now;
    if (!configured_ || wifiManager_ == nullptr) {
        return;
    }

    if (!started_ && wifiManager_->connected()) {
        start();
    }

#if defined(ARDUINO) && !defined(UNIT_TEST) && defined(WITH_ARDUINO_OTA)
    if (started_) {
        ArduinoOTA.handle();
    }
#endif
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
}

} // namespace ewfm
