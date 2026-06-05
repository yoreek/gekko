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

void ArduinoOtaService::begin(const std::string& hostname) {
#if defined(ARDUINO) && !defined(UNIT_TEST) && defined(WITH_ARDUINO_OTA)
    if (started_) {
        return;
    }

    hostname_ = hostname;
    ArduinoOTA.setHostname(hostname_.c_str());
    ArduinoOTA.onStart(onOtaStart);
    ArduinoOTA.onEnd(onOtaEnd);
    ArduinoOTA.onProgress(onOtaProgress);
    ArduinoOTA.onError(onOtaError);
    ArduinoOTA.begin();
    started_ = true;
#else
    (void)hostname;
#endif
}

void ArduinoOtaService::tick() {
#if defined(ARDUINO) && !defined(UNIT_TEST) && defined(WITH_ARDUINO_OTA)
    if (!started_) {
        return;
    }

    ArduinoOTA.handle();
#endif
}

} // namespace ewfm
