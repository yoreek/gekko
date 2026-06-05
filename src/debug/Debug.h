#pragma once

#include "debug/DebugLevel.h"

#include <cstdarg>

namespace ewfm {

class DebugLogger {
public:
    void begin();
    void setLevel(DebugLevel level) {
        level_ = level;
    }
    DebugLevel level() const {
        return level_;
    }
    bool enabled(DebugLevel level) const;
    void log(DebugLevel level, const char* domain, const char* file, int line, const char* function, const char* format, ...);
    void vlog(DebugLevel level, const char* domain, const char* file, int line, const char* function, const char* format, va_list args);

private:
    DebugLevel level_{DebugLevel::Info};
};

DebugLogger& debugLogger();

} // namespace ewfm

#if defined(WITH_DEBUG)
#ifndef DEBUG_LEVEL
#define DEBUG_LEVEL Info
#endif

#define EWFM_DEBUG_BEGIN()                                                                                                                 \
    do {                                                                                                                                   \
        ewfm::debugLogger().setLevel(ewfm::DebugLevel::DEBUG_LEVEL);                                                                       \
        ewfm::debugLogger().begin();                                                                                                       \
    } while (0)

#define EWFM_LOG_LEVEL(level, domain, fmt, ...)                                                                                            \
    do {                                                                                                                                   \
        ewfm::debugLogger().log(ewfm::DebugLevel::level, domain, __FILE__, __LINE__, __FUNCTION__, fmt, ##__VA_ARGS__);                    \
    } while (0)

#define EWFM_LOG_ERROR(domain, fmt, ...) EWFM_LOG_LEVEL(Error, domain, fmt, ##__VA_ARGS__)
#define EWFM_LOG_WARN(domain, fmt, ...) EWFM_LOG_LEVEL(Warn, domain, fmt, ##__VA_ARGS__)
#define EWFM_LOG_INFO(domain, fmt, ...) EWFM_LOG_LEVEL(Info, domain, fmt, ##__VA_ARGS__)
#define EWFM_LOG_DEBUG(domain, fmt, ...) EWFM_LOG_LEVEL(Debug, domain, fmt, ##__VA_ARGS__)
#define EWFM_LOG_TRACE(domain, fmt, ...) EWFM_LOG_LEVEL(Trace, domain, fmt, ##__VA_ARGS__)
#else
#define EWFM_DEBUG_BEGIN()
#define EWFM_LOG_ERROR(domain, fmt, ...)
#define EWFM_LOG_WARN(domain, fmt, ...)
#define EWFM_LOG_INFO(domain, fmt, ...)
#define EWFM_LOG_DEBUG(domain, fmt, ...)
#define EWFM_LOG_TRACE(domain, fmt, ...)
#endif

#if defined(WITH_APP_STATE_MANAGER_DEBUG)
#define EWFM_APP_LOG_INFO(fmt, ...) EWFM_LOG_INFO("app", fmt, ##__VA_ARGS__)
#define EWFM_APP_LOG_DEBUG(fmt, ...) EWFM_LOG_DEBUG("app", fmt, ##__VA_ARGS__)
#else
#define EWFM_APP_LOG_INFO(fmt, ...)
#define EWFM_APP_LOG_DEBUG(fmt, ...)
#endif

#if defined(WITH_WIFI_NETWORK_MANAGER_DEBUG)
#define EWFM_WIFI_LOG_INFO(fmt, ...) EWFM_LOG_INFO("wifi", fmt, ##__VA_ARGS__)
#define EWFM_WIFI_LOG_WARN(fmt, ...) EWFM_LOG_WARN("wifi", fmt, ##__VA_ARGS__)
#define EWFM_WIFI_LOG_DEBUG(fmt, ...) EWFM_LOG_DEBUG("wifi", fmt, ##__VA_ARGS__)
#else
#define EWFM_WIFI_LOG_INFO(fmt, ...)
#define EWFM_WIFI_LOG_WARN(fmt, ...)
#define EWFM_WIFI_LOG_DEBUG(fmt, ...)
#endif

#if defined(WITH_CONFIG_STORE_DEBUG)
#define EWFM_CONFIG_LOG_INFO(fmt, ...) EWFM_LOG_INFO("config", fmt, ##__VA_ARGS__)
#define EWFM_CONFIG_LOG_WARN(fmt, ...) EWFM_LOG_WARN("config", fmt, ##__VA_ARGS__)
#else
#define EWFM_CONFIG_LOG_INFO(fmt, ...)
#define EWFM_CONFIG_LOG_WARN(fmt, ...)
#endif

#if defined(WITH_PORTAL_SERVER_DEBUG)
#define EWFM_PORTAL_LOG_INFO(fmt, ...) EWFM_LOG_INFO("portal", fmt, ##__VA_ARGS__)
#define EWFM_PORTAL_LOG_WARN(fmt, ...) EWFM_LOG_WARN("portal", fmt, ##__VA_ARGS__)
#define EWFM_PORTAL_LOG_DEBUG(fmt, ...) EWFM_LOG_DEBUG("portal", fmt, ##__VA_ARGS__)
#else
#define EWFM_PORTAL_LOG_INFO(fmt, ...)
#define EWFM_PORTAL_LOG_WARN(fmt, ...)
#define EWFM_PORTAL_LOG_DEBUG(fmt, ...)
#endif

#if defined(WITH_PROVISIONING_DEBUG)
#define EWFM_PROV_LOG_INFO(fmt, ...) EWFM_LOG_INFO("provisioning", fmt, ##__VA_ARGS__)
#define EWFM_PROV_LOG_WARN(fmt, ...) EWFM_LOG_WARN("provisioning", fmt, ##__VA_ARGS__)
#else
#define EWFM_PROV_LOG_INFO(fmt, ...)
#define EWFM_PROV_LOG_WARN(fmt, ...)
#endif

#if defined(WITH_STATE_MACHINE_DEBUG)
#define EWFM_SM_LOG_DEBUG(fmt, ...) EWFM_LOG_DEBUG("state-machine", fmt, ##__VA_ARGS__)
#else
#define EWFM_SM_LOG_DEBUG(fmt, ...)
#endif
