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
