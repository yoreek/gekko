#pragma once

#include "core/StateMachine.h"

#include <ArduinoJson.h>
#include <cstdint>

namespace ewfm {

struct BusRuntimeDiagnostics {
    uint32_t consecutiveErrors{0};
    uint32_t lastErrorCode{0};
    uint32_t lastErrorAtMs{0};
    uint32_t errorOps{0};

    bool recordError(uint32_t errorCode, uint32_t now, uint32_t debounceMs) {
        ++errorOps;
        ++consecutiveErrors;
        lastErrorCode = errorCode;
        lastErrorAtMs = now;
        if (!throttled_ || EWFM_SM_TIME_REACHED(now, nextAllowedPublishAtMs_)) {
            throttled_ = true;
            nextAllowedPublishAtMs_ = now + debounceMs;
            return true;
        }
        return false;
    }

    bool recordSuccess() {
        const bool hadErrors = consecutiveErrors != 0U;
        consecutiveErrors = 0U;
        if (hadErrors) {
            throttled_ = false;
            nextAllowedPublishAtMs_ = 0U;
        }
        return hadErrors;
    }

    void reset() {
        consecutiveErrors = 0U;
        lastErrorCode = 0U;
        lastErrorAtMs = 0U;
        errorOps = 0U;
        throttled_ = false;
        nextAllowedPublishAtMs_ = 0U;
    }

    const char* status() const {
        return consecutiveErrors == 0U ? "ok" : "degraded";
    }

    void writeJson(JsonObject output) const {
        JsonObject diagnostics = output.createNestedObject("diagnostics");
        diagnostics["status"] = status();
        diagnostics["consecutiveErrors"] = consecutiveErrors;
        diagnostics["lastErrorCode"] = lastErrorCode;
        diagnostics["lastErrorAtMs"] = lastErrorAtMs;
        diagnostics["errorOps"] = errorOps;
    }

private:
    bool throttled_{false};
    uint32_t nextAllowedPublishAtMs_{0};
};

} // namespace ewfm
