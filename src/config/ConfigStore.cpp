#include "config/ConfigStore.h"

#include "debug/Debug.h"

#if defined(ARDUINO) && !defined(UNIT_TEST)
#include "time/DateTime.h"
#endif

namespace ewfm {

namespace {
constexpr const char* kNamespace = "device";
constexpr const char* kSchema = "schema";
constexpr const char* kDevice = "device";
constexpr const char* kSsid = "ssid";
constexpr const char* kPassword = "password";
constexpr const char* kHttpPortal = "http_portal";
constexpr const char* kMobileProv = "mobile_prov";
constexpr const char* kWebOta = "web_ota";
constexpr const char* kNtpEnabled = "ntp_enabled";
constexpr const char* kNtpServer = "ntp_server";
constexpr const char* kTimezoneId = "tz_id";
constexpr const char* kNtpSyncInterval = "ntp_sync_int";
} // namespace

bool ConfigStore::begin() {
    return storage_.begin(kNamespace, false);
}

ValidationResult ConfigStore::load() {
    config_ = defaultConfig();

    uint32_t schema{config_.schemaVersion};
    if (storage_.getUInt(kSchema, schema)) {
        config_.schemaVersion = schema;
    }
    storage_.getString(kDevice, config_.deviceName);
    storage_.getString(kSsid, config_.wifi.ssid);
    storage_.getString(kPassword, config_.wifi.password);
    storage_.getBool(kHttpPortal, config_.provisioning.httpPortalEnabled);
    storage_.getBool(kMobileProv, config_.provisioning.mobileProvisioningEnabled);
    storage_.getBool(kWebOta, config_.firmwareUpdate.webOtaEnabled);
    storage_.getBool(kNtpEnabled, config_.time.enabled);
    storage_.getString(kNtpServer, config_.time.ntpServer);
    storage_.getString(kTimezoneId, config_.time.timezoneId);
    storage_.getUInt(kNtpSyncInterval, config_.time.syncIntervalSeconds);

    const ValidationResult migrated = migrateConfig(config_);
    if (!migrated.ok()) {
        config_ = defaultConfig();
        return migrated;
    }

    if (config_.schemaVersion == kCurrentConfigSchemaVersion) {
        save(config_);
    }
    return {};
}

ValidationResult ConfigStore::save(const DeviceConfig& config) {
    const ValidationResult validation = validateConfig(config, false);
    if (!validation.ok()) {
        return validation;
    }

    const bool saved = storage_.putUInt(kSchema, config.schemaVersion) && storage_.putString(kDevice, config.deviceName) &&
                       storage_.putString(kSsid, config.wifi.ssid) && storage_.putString(kPassword, config.wifi.password) &&
                       storage_.putBool(kHttpPortal, config.provisioning.httpPortalEnabled) &&
                       storage_.putBool(kMobileProv, config.provisioning.mobileProvisioningEnabled) &&
                       storage_.putBool(kWebOta, config.firmwareUpdate.webOtaEnabled) &&
                       storage_.putBool(kNtpEnabled, config.time.enabled) && storage_.putString(kNtpServer, config.time.ntpServer) &&
                       storage_.putString(kTimezoneId, config.time.timezoneId) &&
                       storage_.putUInt(kNtpSyncInterval, config.time.syncIntervalSeconds);

    if (!saved) {
        EWFM_CONFIG_LOG_WARN("failed to save configuration");
        return {ConfigError::StorageError, "failed to save configuration"};
    }

    config_ = config;
#if defined(ARDUINO) && !defined(UNIT_TEST)
    // The app's current timezone lives here, independent of NTP (which only ever writes the
    // system clock via setTime() - it has no notion of timezone at all). Every config write
    // (boot load, WiFi credential save, time settings save, ...) funnels through this one method,
    // so this is the single place that keeps DateTime::current()'s timezone in sync with settings.
    DateTime::applyTimezone(config_.time.timezoneId.c_str());
#endif
    EWFM_CONFIG_LOG_INFO("configuration saved");
    return {};
}

ValidationResult ConfigStore::saveWifiCredentials(const WiFiCredentials& credentials) {
    const ValidationResult validation = validateWifiCredentials(credentials, true);
    if (!validation.ok()) {
        return validation;
    }
    DeviceConfig next = config_;
    next.wifi = credentials;
    return save(next);
}

ValidationResult ConfigStore::saveTimeConfig(const TimeConfig& time) {
    const ValidationResult validation = validateTimeConfig(time);
    if (!validation.ok()) {
        return validation;
    }
    DeviceConfig next = config_;
    next.time = time;
    return save(next);
}

bool ConfigStore::clearWifiCredentials() {
    config_.wifi = {};
    bool removed = storage_.remove(kSsid);
    removed = storage_.remove(kPassword) && removed;
    return removed && save(config_).ok();
}

} // namespace ewfm
