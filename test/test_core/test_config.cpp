#include "config/ConfigJson.h"
#include "config/ConfigStore.h"
#include "config/MemoryConfigStorage.h"

#include <unity.h>

using namespace ewfm;

void test_default_config_is_valid() {
    DeviceConfig config = defaultConfig();
    ValidationResult result = validateConfig(config);
    TEST_ASSERT_TRUE(result.ok());
    TEST_ASSERT_EQUAL_UINT32(kCurrentConfigSchemaVersion, config.schemaVersion);
    TEST_ASSERT_EQUAL_UINT32(3UL * 60UL * 1000UL, config.provisioning.sessionTimeoutMs);
}

void test_invalid_ssid_is_rejected() {
    WiFiCredentials credentials;
    credentials.ssid.assign(kMaxSsidLength + 1, 'x');
    ValidationResult result = validateWifiCredentials(credentials, true);
    TEST_ASSERT_FALSE(result.ok());
    TEST_ASSERT_EQUAL(static_cast<int>(ConfigError::SsidTooLong), static_cast<int>(result.error));
}

void test_store_does_not_save_invalid_wifi_credentials() {
    MemoryConfigStorage storage;
    ConfigStore store(storage);
    TEST_ASSERT_TRUE(store.begin());
    TEST_ASSERT_TRUE(store.load().ok());

    WiFiCredentials credentials;
    credentials.ssid = "";
    ValidationResult result = store.saveWifiCredentials(credentials);
    TEST_ASSERT_FALSE(result.ok());
    TEST_ASSERT_FALSE(store.config().wifi.hasCredentials());
}

void test_json_export_redacts_password() {
    DeviceConfig config = defaultConfig();
    config.wifi.ssid = "office";
    config.wifi.password = "secret";

    JsonResult result = exportConfigJson(config);
    TEST_ASSERT_TRUE(result.success);
    TEST_ASSERT_TRUE(result.payload.find("office") != std::string::npos);
    TEST_ASSERT_TRUE(result.payload.find("secret") == std::string::npos);
    TEST_ASSERT_TRUE(result.payload.find("password_redacted") != std::string::npos);
}

void test_json_import_rejects_oversized_input() {
    DeviceConfig config = defaultConfig();
    config.maxJsonBytes = 8;
    JsonResult result = importConfigJson("{\"device_name\":\"too-large\"}", config);
    TEST_ASSERT_FALSE(result.success);
    TEST_ASSERT_EQUAL(static_cast<int>(ConfigError::JsonTooLarge), static_cast<int>(result.error));
}

void test_time_config_rejects_unknown_timezone() {
    TimeConfig time = defaultConfig().time;
    time.timezoneId = "Not/AZone";
    ValidationResult result = validateTimeConfig(time);
    TEST_ASSERT_FALSE(result.ok());
    TEST_ASSERT_EQUAL(static_cast<int>(ConfigError::TimezoneInvalid), static_cast<int>(result.error));
}

void test_time_config_rejects_out_of_range_sync_interval() {
    TimeConfig time = defaultConfig().time;
    time.syncIntervalSeconds = kMinNtpSyncIntervalSeconds - 1;
    ValidationResult tooShort = validateTimeConfig(time);
    TEST_ASSERT_FALSE(tooShort.ok());
    TEST_ASSERT_EQUAL(static_cast<int>(ConfigError::SyncIntervalInvalid), static_cast<int>(tooShort.error));

    time.syncIntervalSeconds = kMaxNtpSyncIntervalSeconds + 1;
    ValidationResult tooLong = validateTimeConfig(time);
    TEST_ASSERT_FALSE(tooLong.ok());
    TEST_ASSERT_EQUAL(static_cast<int>(ConfigError::SyncIntervalInvalid), static_cast<int>(tooLong.error));
}

void test_time_config_rejects_oversized_ntp_server() {
    TimeConfig time = defaultConfig().time;
    time.ntpServer.assign(kMaxNtpServerLength + 1, 'x');
    ValidationResult result = validateTimeConfig(time);
    TEST_ASSERT_FALSE(result.ok());
    TEST_ASSERT_EQUAL(static_cast<int>(ConfigError::NtpServerTooLong), static_cast<int>(result.error));
}

void test_config_store_save_time_config_persists_across_reload() {
    MemoryConfigStorage storage;
    ConfigStore store(storage);
    TEST_ASSERT_TRUE(store.begin());
    TEST_ASSERT_TRUE(store.load().ok());

    TimeConfig time;
    time.ntpServer = "time.example.org";
    time.timezoneId = "Europe/Kyiv";
    time.syncIntervalSeconds = 7200;
    TEST_ASSERT_TRUE(store.saveTimeConfig(time).ok());

    TEST_ASSERT_TRUE(store.load().ok());
    TEST_ASSERT_EQUAL_STRING("time.example.org", store.config().time.ntpServer.c_str());
    TEST_ASSERT_EQUAL_STRING("Europe/Kyiv", store.config().time.timezoneId.c_str());
    TEST_ASSERT_EQUAL_UINT32(7200UL, store.config().time.syncIntervalSeconds);
}

void test_config_store_rejects_invalid_time_config() {
    MemoryConfigStorage storage;
    ConfigStore store(storage);
    TEST_ASSERT_TRUE(store.begin());
    TEST_ASSERT_TRUE(store.load().ok());

    TimeConfig invalid;
    invalid.timezoneId = "Not/AZone";
    ValidationResult result = store.saveTimeConfig(invalid);
    TEST_ASSERT_FALSE(result.ok());
    TEST_ASSERT_EQUAL_STRING("Etc/GMT", store.config().time.timezoneId.c_str());
}

void test_persistence_config_rejects_out_of_range_debounce() {
    PersistenceConfig persistence = defaultConfig().persistence;
    persistence.debounceMs = kMinPersistenceDebounceMs - 1;
    ValidationResult tooShort = validatePersistenceConfig(persistence);
    TEST_ASSERT_FALSE(tooShort.ok());
    TEST_ASSERT_EQUAL(static_cast<int>(ConfigError::PersistenceDebounceInvalid), static_cast<int>(tooShort.error));

    persistence.debounceMs = kMaxPersistenceDebounceMs + 1;
    ValidationResult tooLong = validatePersistenceConfig(persistence);
    TEST_ASSERT_FALSE(tooLong.ok());
    TEST_ASSERT_EQUAL(static_cast<int>(ConfigError::PersistenceDebounceInvalid), static_cast<int>(tooLong.error));
}

void test_persistence_config_rejects_out_of_range_max_delay() {
    PersistenceConfig persistence = defaultConfig().persistence;
    persistence.maxDelayMs = kMinPersistenceMaxDelayMs - 1;
    ValidationResult tooShort = validatePersistenceConfig(persistence);
    TEST_ASSERT_FALSE(tooShort.ok());
    TEST_ASSERT_EQUAL(static_cast<int>(ConfigError::PersistenceMaxDelayInvalid), static_cast<int>(tooShort.error));

    persistence.maxDelayMs = kMaxPersistenceMaxDelayMs + 1;
    ValidationResult tooLong = validatePersistenceConfig(persistence);
    TEST_ASSERT_FALSE(tooLong.ok());
    TEST_ASSERT_EQUAL(static_cast<int>(ConfigError::PersistenceMaxDelayInvalid), static_cast<int>(tooLong.error));
}

void test_persistence_config_rejects_debounce_exceeding_max_delay() {
    PersistenceConfig persistence;
    persistence.debounceMs = 5000;
    persistence.maxDelayMs = 1000;
    ValidationResult result = validatePersistenceConfig(persistence);
    TEST_ASSERT_FALSE(result.ok());
    TEST_ASSERT_EQUAL(static_cast<int>(ConfigError::PersistenceDebounceExceedsMaxDelay), static_cast<int>(result.error));
}

void test_config_store_save_persistence_config_persists_across_reload() {
    MemoryConfigStorage storage;
    ConfigStore store(storage);
    TEST_ASSERT_TRUE(store.begin());
    TEST_ASSERT_TRUE(store.load().ok());

    PersistenceConfig persistence;
    persistence.debounceMs = 1000;
    persistence.maxDelayMs = 30000;
    TEST_ASSERT_TRUE(store.savePersistenceConfig(persistence).ok());

    TEST_ASSERT_TRUE(store.load().ok());
    TEST_ASSERT_EQUAL_UINT32(1000UL, store.config().persistence.debounceMs);
    TEST_ASSERT_EQUAL_UINT32(30000UL, store.config().persistence.maxDelayMs);
}

void test_config_store_rejects_invalid_persistence_config() {
    MemoryConfigStorage storage;
    ConfigStore store(storage);
    TEST_ASSERT_TRUE(store.begin());
    TEST_ASSERT_TRUE(store.load().ok());

    PersistenceConfig invalid;
    invalid.debounceMs = kMaxPersistenceDebounceMs + 1;
    ValidationResult result = store.savePersistenceConfig(invalid);
    TEST_ASSERT_FALSE(result.ok());
    TEST_ASSERT_EQUAL_UINT32(kDefaultPersistenceDebounceMs, store.config().persistence.debounceMs);
}

void test_default_config_has_default_board_model() {
    DeviceConfig config = defaultConfig();
    TEST_ASSERT_EQUAL(static_cast<int>(kDefaultBoardModel), static_cast<int>(config.boardModel));
}

void test_config_store_save_board_model_persists_across_reload() {
    MemoryConfigStorage storage;
    ConfigStore store(storage);
    TEST_ASSERT_TRUE(store.begin());
    TEST_ASSERT_TRUE(store.load().ok());

    DeviceConfig next = store.config();
    next.boardModel = static_cast<BoardModel>((static_cast<size_t>(kDefaultBoardModel) + 1U) % kSupportedBoardIdCount);
    TEST_ASSERT_TRUE(store.save(next).ok());

    TEST_ASSERT_TRUE(store.load().ok());
    TEST_ASSERT_EQUAL(static_cast<int>(next.boardModel), static_cast<int>(store.config().boardModel));
}

void test_config_store_resets_out_of_range_board_model_to_default() {
    MemoryConfigStorage storage;
    ConfigStore store(storage);
    TEST_ASSERT_TRUE(store.begin());
    TEST_ASSERT_TRUE(store.load().ok());

    DeviceConfig corrupted = store.config();
    corrupted.boardModel = static_cast<BoardModel>(kSupportedBoardIdCount + 10U);
    ValidationResult migrated = migrateConfig(corrupted);
    TEST_ASSERT_TRUE(migrated.ok());
    TEST_ASSERT_EQUAL(static_cast<int>(kDefaultBoardModel), static_cast<int>(corrupted.boardModel));
}
