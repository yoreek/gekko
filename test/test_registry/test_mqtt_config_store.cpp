#include "config/MemoryConfigStorage.h"
#include "config/MqttConfigStore.h"
#include "devices/registry/DeviceScopedDataStore.h"
#include "integrations/mqtt/HaDeviceSettings.h"

#include <unity.h>
#include <vector>

using namespace ewfm;

void test_mqtt_config_store_round_trips_settings() {
    MemoryConfigStorage storage;
    MqttConfigStore store(storage);
    TEST_ASSERT_TRUE(store.begin());

    MqttSettings settings = defaultMqttSettings();
    settings.enabled = true;
    settings.host = "broker.local";
    settings.port = 8883;
    settings.useTls = true;
    settings.clientId = "node1";
    settings.username = "user";
    settings.password = "pass";
    settings.haDiscoveryPrefix = "homeassistant";
    settings.haNodeId = "node1-abc123";
    settings.haNodeName = "Node One";

    TEST_ASSERT_TRUE(store.save(settings).ok());

    MqttConfigStore reloaded(storage);
    TEST_ASSERT_TRUE(reloaded.begin());
    TEST_ASSERT_TRUE(reloaded.load().ok());
    TEST_ASSERT_TRUE(reloaded.settings().enabled);
    TEST_ASSERT_EQUAL_STRING("broker.local", reloaded.settings().host.c_str());
    TEST_ASSERT_EQUAL_UINT16(8883, reloaded.settings().port);
    TEST_ASSERT_TRUE(reloaded.settings().useTls);
    TEST_ASSERT_EQUAL_STRING("node1", reloaded.settings().clientId.c_str());
    TEST_ASSERT_EQUAL_STRING("user", reloaded.settings().username.c_str());
    TEST_ASSERT_EQUAL_STRING("pass", reloaded.settings().password.c_str());
    TEST_ASSERT_EQUAL_STRING("node1-abc123", reloaded.settings().haNodeId.c_str());
    TEST_ASSERT_EQUAL_STRING("Node One", reloaded.settings().haNodeName.c_str());
}

void test_mqtt_config_store_rejects_enabled_without_host_or_client_id() {
    MemoryConfigStorage storage;
    MqttConfigStore store(storage);
    TEST_ASSERT_TRUE(store.begin());

    MqttSettings settings = defaultMqttSettings();
    settings.enabled = true;
    settings.haNodeId = "node1";

    const MqttConfigValidationResult missingHost = store.save(settings);
    TEST_ASSERT_FALSE(missingHost.ok());
    TEST_ASSERT_EQUAL_INT(static_cast<int>(MqttConfigError::HostRequired), static_cast<int>(missingHost.error));

    settings.host = "broker.local";
    const MqttConfigValidationResult missingClientId = store.save(settings);
    TEST_ASSERT_FALSE(missingClientId.ok());
    TEST_ASSERT_EQUAL_INT(static_cast<int>(MqttConfigError::ClientIdRequired), static_cast<int>(missingClientId.error));
}

void test_mqtt_config_store_rejects_invalid_node_id() {
    MemoryConfigStorage storage;
    MqttConfigStore store(storage);
    TEST_ASSERT_TRUE(store.begin());

    MqttSettings settings = defaultMqttSettings();
    settings.haNodeId = "";
    TEST_ASSERT_EQUAL_INT(static_cast<int>(MqttConfigError::NodeIdRequired), static_cast<int>(store.save(settings).error));

    settings.haNodeId = "node with spaces";
    TEST_ASSERT_EQUAL_INT(static_cast<int>(MqttConfigError::NodeIdInvalidCharacters), static_cast<int>(store.save(settings).error));

    settings.haNodeId = "valid-node_1";
    TEST_ASSERT_TRUE(store.save(settings).ok());
}

void test_mqtt_config_store_round_trips_ca_cert() {
    MemoryConfigStorage storage;
    MqttConfigStore store(storage);
    TEST_ASSERT_TRUE(store.begin());

    TEST_ASSERT_FALSE(store.hasCaCert());

    const std::vector<uint8_t> pem = {'-', '-', '-', 'B', 'E', 'G', 'I', 'N', '-', '-', '-'};
    TEST_ASSERT_TRUE(store.saveCaCert(pem.data(), pem.size()).ok());
    TEST_ASSERT_TRUE(store.hasCaCert());

    std::vector<uint8_t> loaded;
    TEST_ASSERT_TRUE(store.loadCaCert(loaded).ok());
    TEST_ASSERT_TRUE(loaded == pem);

    TEST_ASSERT_TRUE(store.clearCaCert());
    TEST_ASSERT_FALSE(store.hasCaCert());
}

void test_mqtt_config_store_rejects_oversized_ca_cert() {
    MemoryConfigStorage storage;
    MqttConfigStore store(storage);
    TEST_ASSERT_TRUE(store.begin());

    const std::vector<uint8_t> oversized(kMqttMaxCaCertBytes + 1, 'x');
    const MqttConfigValidationResult result = store.saveCaCert(oversized.data(), oversized.size());
    TEST_ASSERT_FALSE(result.ok());
    TEST_ASSERT_EQUAL_INT(static_cast<int>(MqttConfigError::CertTooLarge), static_cast<int>(result.error));
}

void test_default_ha_node_id_is_sanitized_and_bounded() {
    TEST_ASSERT_EQUAL_STRING("esp32_wifi_manager-ABC123", defaultHaNodeId("esp32 wifi manager", "ABC123").c_str());
    TEST_ASSERT_TRUE(isValidHaNodeIdCharset(defaultHaNodeId("esp32 wifi manager", "ABC123")));
    TEST_ASSERT_TRUE(isValidHaNodeIdCharset(defaultHaNodeId("<no name>", "ABC123")));
    TEST_ASSERT_EQUAL_STRING("esp32-setup", defaultHaNodeId("esp32", "").c_str());
}

void test_ha_device_settings_round_trip_and_default() {
    MemoryConfigStorage storage;
    DeviceScopedDataStore store(storage);
    TEST_ASSERT_TRUE(store.begin(false));

    const HaDeviceSettingsRecord missing = loadHaDeviceSettings(store, 42);
    TEST_ASSERT_FALSE(missing.enabled);
    TEST_ASSERT_EQUAL_STRING("", missing.nameOverride);

    TEST_ASSERT_TRUE(saveHaDeviceSettings(store, 42, true, "Pump").ok());
    const HaDeviceSettingsRecord loaded = loadHaDeviceSettings(store, 42);
    TEST_ASSERT_TRUE(loaded.enabled);
    TEST_ASSERT_EQUAL_STRING("Pump", loaded.nameOverride);
    TEST_ASSERT_EQUAL_STRING("Pump", effectiveHaDeviceName(loaded, "fallback").c_str());

    TEST_ASSERT_TRUE(saveHaDeviceSettings(store, 42, false, "").ok());
    const HaDeviceSettingsRecord cleared = loadHaDeviceSettings(store, 42);
    TEST_ASSERT_FALSE(cleared.enabled);
    TEST_ASSERT_EQUAL_STRING("fallback", effectiveHaDeviceName(cleared, "fallback").c_str());
}
