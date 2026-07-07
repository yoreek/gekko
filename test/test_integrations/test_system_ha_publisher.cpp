#include "config/MemoryConfigStorage.h"
#include "core/SystemStats.h"
#include "devices/core/DeviceIdGenerator.h"
#include "devices/dummy/DummyDevice.h"
#include "devices/registry/DeviceRegistry.h"
#include "integrations/mqtt/system/SystemHaPublisher.h"
#include "platform/MqttManager.h"
#include "wifi/WifiManager.h"

#include <cstdio>
#include <unity.h>

using namespace ewfm;

namespace {

class FakeWifiDriver final : public IWifiDriver {
public:
    bool begin() override {
        networkStackReadyValue = true;
        return true;
    }
    bool beginStation(const WiFiCredentials& credentials) override {
        (void)credentials;
        networkStackReadyValue = true;
        statusValue = WifiDriverStatus::Connecting;
        return true;
    }
    void disconnect() override {}
    void clearStationCredentials() override {
        statusValue = WifiDriverStatus::Idle;
    }
    bool startSetupAp(const std::string& ssid, const std::string& password) override {
        (void)ssid;
        (void)password;
        return false;
    }
    void stopSetupAp() override {}
    WifiDriverStatus status() const override {
        return statusValue;
    }
    bool networkStackReady() const override {
        return networkStackReadyValue;
    }
    bool stationReady() const override {
        return networkStackReadyValue && statusValue == WifiDriverStatus::Connected && !stationIpValue.empty() &&
               stationIpValue != "0.0.0.0";
    }
    bool setupApReady() const override {
        return false;
    }
    std::string stationIp() const override {
        return stationIpValue;
    }
    std::string setupApIp() const override {
        return {};
    }
    bool startScan() override {
        return true;
    }
    bool scanComplete(std::vector<WifiNetwork>& networks, size_t maxResults) override {
        (void)networks;
        (void)maxResults;
        return false;
    }
    std::string macSuffix() const override {
        return "ABC123";
    }
    int32_t rssi() const override {
        return rssiValue;
    }
    std::string ssid() const override {
        return ssidValue;
    }

    WifiDriverStatus statusValue{WifiDriverStatus::Idle};
    bool networkStackReadyValue{false};
    std::string stationIpValue{};
    int32_t rssiValue{-57};
    std::string ssidValue{"office"};
};

BoundedBlob<kMaxDeviceConfigBytes> encodeDummyPayload(const char* name) {
    DummyDeviceConfigV1 config{};
    config.enabled = true;
    std::snprintf(config.name, sizeof(config.name), "%s", name);
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    BoundedBlob<kMaxDeviceConfigBytes> payload{};
    TEST_ASSERT_TRUE(encodeDummyDeviceConfig(config, buffer, dummyDeviceConfigSize(config)));
    TEST_ASSERT_TRUE(payload.assign(buffer, dummyDeviceConfigSize(config)));
    return payload;
}

DeviceCreateRequest makeDummyCreateRequest(const char* name) {
    DeviceCreateRequest request{};
    request.typeId = DummyDevice::descriptor().typeId;
    request.name = name;
    request.enabled = true;
    request.configVersion = DummyDevice::descriptor().currentConfigVersion;
    request.configBlob = encodeDummyPayload(name);
    return request;
}

std::string publishedPayloadFor(const MqttManager& mqtt, const std::string& topic) {
    for (const auto& message : mqtt.publishedMessages()) {
        if (message.topic == topic) {
            return message.payload;
        }
    }
    return "<missing>";
}

bool anyDiscoveryContains(const MqttManager& mqtt, const std::string& topic, const std::string& needle) {
    for (const auto& message : mqtt.publishedMessages()) {
        if (message.topic == topic) {
            return message.payload.find(needle) != std::string::npos;
        }
    }
    return false;
}

// Fixture: brings up a WifiManager+MqttManager pair to the Connected state, mirroring
// test_ha_discovery_bridge.cpp's fixture, but for SystemHaPublisher instead.
struct Fixture {
    FakeWifiDriver driver;
    WifiManager wifiManager{driver};
    MqttManager mqtt;
    ManualSystemStats systemStats;
    MemoryConfigStorage registryStorage;
    DeviceRegistryStore registryStore{registryStorage};
    SequentialDeviceIdSource ids{1000};
    DeviceTypeRegistry types{DeviceTypeRegistry::withDefaults()};
    DeviceRegistry registry{registryStore, types, ids};
    SystemHaPublisher publisher{&mqtt, &wifiManager, &systemStats, &registry};

    Fixture() {
        TEST_ASSERT_TRUE(registryStore.begin(false));
        TEST_ASSERT_TRUE(registry.begin(1).ok());

        DeviceConfig config = defaultConfig();
        config.wifi.ssid = "office";
        config.wifi.password = "secret";
        wifiManager.begin(config);

        mqtt.begin(wifiManager);
        MqttSettings settings = defaultMqttSettings();
        settings.enabled = true;
        settings.host = "broker.local";
        settings.clientId = "node1";
        mqtt.applySettings(settings, {});

        publisher.begin("node1", "Node One", "homeassistant");
    }

    void connectMqtt() {
        for (uint32_t t = 100; t < 102; ++t) {
            wifiManager.tick(t);
            mqtt.tick(t);
            publisher.tick(t);
        }
        driver.statusValue = WifiDriverStatus::Connected;
        driver.stationIpValue = "192.168.1.240";
        for (uint32_t t = 102; t < 110; ++t) {
            wifiManager.tick(t);
            mqtt.tick(t);
            publisher.tick(t);
        }
        TEST_ASSERT_TRUE_MESSAGE(mqtt.connected(), "mqtt did not reach Connected state");
    }
};

} // namespace

void test_system_ha_publisher_publishes_discovery_and_state_on_connect() {
    Fixture fx;
    fx.systemStats.set(123456, 12);
    fx.driver.rssiValue = -55;
    fx.driver.ssidValue = "office";
    fx.connectMqtt();

    TEST_ASSERT_TRUE(
        anyDiscoveryContains(fx.mqtt, "homeassistant/sensor/node1/node1_system_uptime/config", "\"device_class\":\"duration\""));
    TEST_ASSERT_TRUE(anyDiscoveryContains(fx.mqtt, "homeassistant/sensor/node1/node1_system_uptime/config",
                                          "\"state_topic\":\"node1/system/uptime/state\""));
    TEST_ASSERT_TRUE(
        anyDiscoveryContains(fx.mqtt, "homeassistant/sensor/node1/node1_system_wifi_rssi/config", "\"device_class\":\"signal_strength\""));
    TEST_ASSERT_TRUE(
        anyDiscoveryContains(fx.mqtt, "homeassistant/button/node1/node1_system_restart/config", "\"device_class\":\"restart\""));
    TEST_ASSERT_TRUE(anyDiscoveryContains(fx.mqtt, "homeassistant/button/node1/node1_system_restart/config",
                                          "\"command_topic\":\"node1/system/restart/set\""));

    TEST_ASSERT_EQUAL_STRING("123456", publishedPayloadFor(fx.mqtt, "node1/system/free_heap/state").c_str());
    TEST_ASSERT_EQUAL_STRING("12", publishedPayloadFor(fx.mqtt, "node1/system/heap_fragmentation/state").c_str());
    TEST_ASSERT_EQUAL_STRING("-55", publishedPayloadFor(fx.mqtt, "node1/system/wifi_rssi/state").c_str());
    TEST_ASSERT_EQUAL_STRING("office", publishedPayloadFor(fx.mqtt, "node1/system/wifi_ssid/state").c_str());
    TEST_ASSERT_EQUAL_STRING("192.168.1.240", publishedPayloadFor(fx.mqtt, "node1/system/wifi_ip/state").c_str());
}

void test_system_ha_publisher_republishes_state_periodically() {
    Fixture fx;
    fx.systemStats.set(1000, 0);
    fx.connectMqtt();
    TEST_ASSERT_EQUAL_STRING("1000", publishedPayloadFor(fx.mqtt, "node1/system/free_heap/state").c_str());

    fx.mqtt.clearPublishedMessages();
    fx.systemStats.set(2000, 5);
    fx.wifiManager.tick(115);
    fx.mqtt.tick(115);
    fx.publisher.tick(115);
    TEST_ASSERT_EQUAL_STRING_MESSAGE("<missing>", publishedPayloadFor(fx.mqtt, "node1/system/free_heap/state").c_str(),
                                     "must not republish before the periodic interval elapses");

    fx.wifiManager.tick(40000);
    fx.mqtt.tick(40000);
    fx.publisher.tick(40000);
    TEST_ASSERT_EQUAL_STRING("2000", publishedPayloadFor(fx.mqtt, "node1/system/free_heap/state").c_str());
}

void test_system_ha_publisher_restart_command_flushes_pending_persistence() {
    Fixture fx;
    DeviceCreateResult created = fx.registry.create(makeDummyCreateRequest("dummy"), 10);
    TEST_ASSERT_TRUE_MESSAGE(created.ok(), created.validation.message);
    fx.connectMqtt();

    const std::array<DeviceDependencyLink, kMaxDeviceDependencies> noDeps{};
    DeviceMutationResult renamed =
        fx.registry.updateConfigAndDeps(created.deviceId, encodeDummyPayload("dummy-renamed"), 0, "dummy-renamed", true, false, noDeps, 0,
                                        20, DevicePersistencePolicy::Delayed);
    TEST_ASSERT_TRUE_MESSAGE(renamed.ok(), renamed.validation.message);
    TEST_ASSERT_TRUE(fx.registry.hasPendingPersistence());

    fx.mqtt.simulateIncomingMessage("node1/system/restart/set", "PRESS");
    TEST_ASSERT_FALSE_MESSAGE(fx.registry.hasPendingPersistence(), "restart command must flush pending persistence before rebooting");
}

void test_system_ha_publisher_ignores_unrelated_topics() {
    Fixture fx;
    DeviceCreateResult created = fx.registry.create(makeDummyCreateRequest("dummy"), 10);
    TEST_ASSERT_TRUE_MESSAGE(created.ok(), created.validation.message);
    fx.connectMqtt();

    const std::array<DeviceDependencyLink, kMaxDeviceDependencies> noDeps{};
    DeviceMutationResult renamed =
        fx.registry.updateConfigAndDeps(created.deviceId, encodeDummyPayload("dummy-renamed"), 0, "dummy-renamed", true, false, noDeps, 0,
                                        20, DevicePersistencePolicy::Delayed);
    TEST_ASSERT_TRUE_MESSAGE(renamed.ok(), renamed.validation.message);
    TEST_ASSERT_TRUE(fx.registry.hasPendingPersistence());

    fx.mqtt.simulateIncomingMessage("node1/switch/5/set", "ON");
    TEST_ASSERT_TRUE_MESSAGE(fx.registry.hasPendingPersistence(), "unrelated topics must not trigger a restart flush");
}
