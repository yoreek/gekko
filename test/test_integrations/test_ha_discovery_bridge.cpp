#include "config/MemoryConfigStorage.h"
#include "core/Clock.h"
#include "devices/bus/i2c/I2cBusDevice.h"
#include "devices/core/DeviceIdGenerator.h"
#include "devices/registry/DeviceRegistry.h"
#include "devices/registry/DeviceScopedDataStore.h"
#include "devices/sensors/htu21/Htu21SensorConfig.h"
#include "devices/sensors/htu21/Htu21SensorDevice.h"
#include "devices/switch/SwitchOutputState.h"
#include "devices/switch/gpio/GpioSwitchDevice.h"
#include "integrations/common/DeviceEventDispatcher.h"
#include "integrations/mqtt/HaDeviceSettings.h"
#include "integrations/mqtt/HaDiscoveryBridge.h"
#include "integrations/mqtt/HaDiscoveryConstants.h"
#include "integrations/mqtt/HaDiscoveryPayload.h"
#include "platform/MqttManager.h"
#include "wifi/WifiManager.h"

#include <ArduinoJson.h>
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

    WifiDriverStatus statusValue{WifiDriverStatus::Idle};
    bool networkStackReadyValue{false};
    std::string stationIpValue{};
};

BoundedBlob<kMaxDeviceConfigBytes> encodeGpioPayload(const GpioSwitchDeviceConfigV3& config) {
    BoundedBlob<kMaxDeviceConfigBytes> payload{};
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    TEST_ASSERT_TRUE(encodeFixedConfigBlob(GpioSwitchDeviceConfigV3::kMagic, config, buffer, gpioSwitchDeviceConfigSize(config)));
    TEST_ASSERT_TRUE(payload.assign(buffer, gpioSwitchDeviceConfigSize(config)));
    return payload;
}

DeviceCreateRequest makeGpioSwitchCreateRequest(const char* name, const bool enabled = true) {
    GpioSwitchDeviceConfigV3 config{};
    config.enabled = enabled;
    std::snprintf(config.name, sizeof(config.name), "%s", name);
    config.restorePreviousState = false;
    config.startupState = kSwitchOutputOff;
    config.safeState = kSwitchOutputOff;
    config.inverted = false;
    config.gpioPin = 13;

    DeviceCreateRequest request{};
    request.typeId = GpioSwitchDevice::descriptor().typeId;
    TEST_ASSERT_TRUE(request.assignName(name));
    request.configBlob = encodeGpioPayload(config);
    request.configVersion = GpioSwitchDevice::descriptor().currentConfigVersion;
    request.setEnabled(enabled);
    return request;
}

bool anyPublishedTopicEquals(const MqttManager& mqtt, const std::string& topic, const std::string& payload) {
    for (const auto& message : mqtt.publishedMessages()) {
        if (message.topic == topic && message.payload == payload) {
            return true;
        }
    }
    return false;
}

bool anyPublishedTopic(const MqttManager& mqtt, const std::string& topic) {
    for (const auto& message : mqtt.publishedMessages()) {
        if (message.topic == topic) {
            return true;
        }
    }
    return false;
}

I2cBusDeviceConfigV1 makeBusConfig() {
    I2cBusDeviceConfigV1 config{};
    config.enabled = 1U;
    std::snprintf(config.name, sizeof(config.name), "%s", "i2c-bus");
    config.sdaPin = 18;
    config.sclPin = 19;
    config.internalPullup = 1U;
    config.frequencyHz = 400000U;
    return config;
}

BoundedBlob<kMaxDeviceConfigBytes> encodeBusPayload(const I2cBusDeviceConfigV1& config) {
    BoundedBlob<kMaxDeviceConfigBytes> payload{};
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    TEST_ASSERT_TRUE(encodeFixedConfigBlob(I2cBusDeviceConfigV1::kMagic, config, buffer, i2cBusDeviceConfigSize(config)));
    TEST_ASSERT_TRUE(payload.assign(buffer, i2cBusDeviceConfigSize(config)));
    return payload;
}

Htu21SensorConfigV3 makeHtu21Config() {
    Htu21SensorConfigV3 config{};
    config.enabled = 1U;
    std::snprintf(config.name, sizeof(config.name), "%s", "climate");
    return config;
}

BoundedBlob<kMaxDeviceConfigBytes> encodeHtu21Payload(const Htu21SensorConfigV3& config) {
    BoundedBlob<kMaxDeviceConfigBytes> payload{};
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    TEST_ASSERT_TRUE(encodeFixedConfigBlob(Htu21SensorConfigV3::kMagic, config, buffer, htu21SensorConfigSize(config)));
    TEST_ASSERT_TRUE(payload.assign(buffer, htu21SensorConfigSize(config)));
    return payload;
}

DeviceCreateRequest makeBusCreateRequest(const char* name) {
    DeviceCreateRequest request{};
    request.typeId = I2cBusDevice::descriptor().typeId;
    TEST_ASSERT_TRUE(request.assignName(name));
    request.setEnabled(true);
    request.configVersion = I2cBusDevice::descriptor().currentConfigVersion;
    request.configBlob = encodeBusPayload(makeBusConfig());
    return request;
}

DeviceCreateRequest makeHtu21CreateRequest(const char* name, DeviceId busId) {
    DeviceCreateRequest request{};
    request.typeId = kHtu21SensorTypeId;
    TEST_ASSERT_TRUE(request.assignName(name));
    request.setEnabled(true);
    request.depCount = 1;
    request.deps[0] = {DeviceRole::I2CBus, busId};
    request.configVersion = kHtu21SensorConfigVersion;
    request.configBlob = encodeHtu21Payload(makeHtu21Config());
    return request;
}

// Fixture: brings up a WifiManager+MqttManager pair to the Connected state so the bridge's birth
// sequence (availability publish + wildcard subscribe + discovery republish) can be observed.
struct Fixture {
    FakeWifiDriver driver;
    WifiManager wifiManager{driver};
    MqttManager mqtt;
    MemoryConfigStorage haStorage;
    DeviceScopedDataStore haSettingsStore{haStorage};
    MemoryConfigStorage registryStorage;
    DeviceRegistryStore registryStore{registryStorage};
    SequentialDeviceIdSource ids{900};
    DeviceTypeRegistry types{DeviceTypeRegistry::withDefaults()};
    DeviceEventDispatcher dispatcher;
    DeviceRegistry registry{registryStore, types, ids, nullptr, nullptr, &dispatcher};
    HaDiscoveryBridge bridge{&mqtt, &registry, &dispatcher, &haSettingsStore};

    Fixture() {
        TEST_ASSERT_TRUE(haSettingsStore.begin(false));
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

        bridge.begin("node1", "Node One", "homeassistant");
        bridge.attachDispatcher();
    }

    void connectMqtt() {
        for (uint32_t t = 100; t < 102; ++t) {
            wifiManager.tick(t);
            mqtt.tick(t);
        }
        driver.statusValue = WifiDriverStatus::Connected;
        driver.stationIpValue = "192.168.1.240";
        for (uint32_t t = 102; t < 110; ++t) {
            wifiManager.tick(t);
            mqtt.tick(t);
        }
        TEST_ASSERT_TRUE_MESSAGE(mqtt.connected(), "mqtt did not reach Connected state");
    }
};

} // namespace

void test_ha_discovery_bridge_publishes_birth_and_subscribes_on_connect() {
    Fixture fx;
    fx.connectMqtt();

    TEST_ASSERT_TRUE(anyPublishedTopicEquals(fx.mqtt, "node1/status", "online"));
    bool subscribed = false;
    for (const auto& topic : fx.mqtt.subscribedTopics()) {
        if (topic == "node1/+/+/set") {
            subscribed = true;
        }
    }
    TEST_ASSERT_TRUE(subscribed);
}

void test_ha_discovery_bridge_skips_unopted_in_device_on_create() {
    Fixture fx;
    fx.connectMqtt();
    fx.mqtt.clearPublishedMessages();

    DeviceCreateResult created = fx.registry.create(makeGpioSwitchCreateRequest("Pump"), 10);
    TEST_ASSERT_TRUE_MESSAGE(created.ok(), created.validation.message);
    fx.dispatcher.tickFastLoop(11);

    // A never-opted-in device may still see a harmless empty retract publish (idempotent no-op
    // for HA - there was nothing to remove), but must never publish actual discovery content.
    const std::string discoveryTopic = "homeassistant/switch/node1/node1_gpio_switch_" + std::to_string(created.deviceId) + "/config";
    for (const auto& message : fx.mqtt.publishedMessages()) {
        if (message.topic == discoveryTopic) {
            TEST_ASSERT_TRUE_MESSAGE(message.payload.empty(), "discovery should not publish real content for an opted-out device");
        }
    }
}

void test_ha_discovery_bridge_publishes_discovery_and_state_after_opt_in() {
    Fixture fx;
    fx.connectMqtt();

    DeviceCreateResult created = fx.registry.create(makeGpioSwitchCreateRequest("Pump"), 10);
    TEST_ASSERT_TRUE_MESSAGE(created.ok(), created.validation.message);
    fx.dispatcher.tickFastLoop(11);

    TEST_ASSERT_TRUE(saveHaDeviceSettings(fx.haSettingsStore, created.deviceId, true, "").ok());
    fx.mqtt.clearPublishedMessages();
    fx.bridge.refreshDevice(created.deviceId);

    const std::string discoveryTopic = "homeassistant/switch/node1/node1_gpio_switch_" + std::to_string(created.deviceId) + "/config";
    const std::string stateTopic = "node1/switch/" + std::to_string(created.deviceId) + "/state";
    TEST_ASSERT_TRUE(anyPublishedTopicEquals(fx.mqtt, stateTopic, "OFF"));

    bool foundDiscovery = false;
    for (const auto& message : fx.mqtt.publishedMessages()) {
        if (message.topic == discoveryTopic) {
            foundDiscovery = true;
            DynamicJsonDocument doc(1536);
            TEST_ASSERT_FALSE(deserializeJson(doc, message.payload));
            TEST_ASSERT_TRUE(std::string(doc[ha::key::kUniqueId].as<const char*>()).find("node1_gpio_switch_") == 0);
            TEST_ASSERT_EQUAL_STRING("node1", doc[ha::key::kBaseTopic].as<const char*>());
            const std::string relativeStateTopic = "~/switch/" + std::to_string(created.deviceId) + "/state";
            const std::string relativeCommandTopic = "~/switch/" + std::to_string(created.deviceId) + "/set";
            TEST_ASSERT_EQUAL_STRING(relativeStateTopic.c_str(), doc[ha::key::kStateTopic].as<const char*>());
            TEST_ASSERT_EQUAL_STRING(relativeCommandTopic.c_str(), doc[ha::key::kCommandTopic].as<const char*>());
            TEST_ASSERT_FALSE(doc.containsKey("unique_id"));
            TEST_ASSERT_FALSE(doc.containsKey("state_topic"));
            TEST_ASSERT_LESS_THAN(kMaxHaDiscoveryPayloadBytes, message.payload.size() + 1U);
        }
    }
    TEST_ASSERT_TRUE_MESSAGE(foundDiscovery, "expected a discovery publish after opting in");
}

void test_ha_discovery_bridge_retracts_discovery_on_delete() {
    Fixture fx;
    fx.connectMqtt();

    DeviceCreateResult created = fx.registry.create(makeGpioSwitchCreateRequest("Pump"), 10);
    TEST_ASSERT_TRUE_MESSAGE(created.ok(), created.validation.message);
    fx.dispatcher.tickFastLoop(11);
    TEST_ASSERT_TRUE(saveHaDeviceSettings(fx.haSettingsStore, created.deviceId, true, "").ok());
    fx.bridge.refreshDevice(created.deviceId);
    fx.mqtt.clearPublishedMessages();

    DeviceMutationResult removed = fx.registry.remove(created.deviceId, 20);
    TEST_ASSERT_TRUE_MESSAGE(removed.ok(), removed.validation.message);
    fx.dispatcher.tickFastLoop(21);

    const std::string discoveryTopic = "homeassistant/switch/node1/node1_gpio_switch_" + std::to_string(created.deviceId) + "/config";
    TEST_ASSERT_TRUE(anyPublishedTopicEquals(fx.mqtt, discoveryTopic, ""));
}

void test_ha_discovery_bridge_routes_incoming_command_to_device_registry() {
    Fixture fx;
    fx.connectMqtt();

    DeviceCreateResult created = fx.registry.create(makeGpioSwitchCreateRequest("Pump"), 10);
    TEST_ASSERT_TRUE_MESSAGE(created.ok(), created.validation.message);
    fx.dispatcher.tickFastLoop(11);
    fx.registry.tickFastLoop(11);

    const std::string commandTopic = "node1/switch/" + std::to_string(created.deviceId) + "/set";
    fx.mqtt.simulateIncomingMessage(commandTopic, "ON");

    const auto* runtime = static_cast<const GpioSwitchDevice*>(fx.registry.runtime(created.deviceId));
    TEST_ASSERT_NOT_NULL(runtime);
    TEST_ASSERT_TRUE(runtime->currentOutputState() == kSwitchOutputOn);
}

void test_ha_discovery_bridge_excludes_disabled_device() {
    Fixture fx;
    fx.connectMqtt();

    DeviceCreateResult created = fx.registry.create(makeGpioSwitchCreateRequest("Disabled pump", false), 10);
    TEST_ASSERT_TRUE_MESSAGE(created.ok(), created.validation.message);
    fx.dispatcher.tickFastLoop(11);
    TEST_ASSERT_TRUE(saveHaDeviceSettings(fx.haSettingsStore, created.deviceId, true, "").ok());

    fx.mqtt.clearPublishedMessages();
    fx.bridge.refreshDevice(created.deviceId);

    const std::string discoveryTopic = "homeassistant/switch/node1/node1_gpio_switch_" + std::to_string(created.deviceId) + "/config";
    const std::string stateTopic = "node1/switch/" + std::to_string(created.deviceId) + "/state";
    TEST_ASSERT_TRUE(anyPublishedTopicEquals(fx.mqtt, discoveryTopic, ""));
    TEST_ASSERT_FALSE(anyPublishedTopic(fx.mqtt, stateTopic));

    const std::string commandTopic = "node1/switch/" + std::to_string(created.deviceId) + "/set";
    fx.mqtt.simulateIncomingMessage(commandTopic, "ON");
    const auto* runtime = static_cast<const GpioSwitchDevice*>(fx.registry.runtime(created.deviceId));
    TEST_ASSERT_NOT_NULL(runtime);
    TEST_ASSERT_FALSE(runtime->currentOutputState());
}

void test_ha_discovery_bridge_retracts_disabled_device_on_connect() {
    Fixture fx;
    DeviceCreateResult created = fx.registry.create(makeGpioSwitchCreateRequest("Disabled pump", false), 10);
    TEST_ASSERT_TRUE_MESSAGE(created.ok(), created.validation.message);
    TEST_ASSERT_TRUE(saveHaDeviceSettings(fx.haSettingsStore, created.deviceId, true, "").ok());

    fx.connectMqtt();

    const std::string discoveryTopic = "homeassistant/switch/node1/node1_gpio_switch_" + std::to_string(created.deviceId) + "/config";
    const std::string stateTopic = "node1/switch/" + std::to_string(created.deviceId) + "/state";
    TEST_ASSERT_TRUE(anyPublishedTopicEquals(fx.mqtt, discoveryTopic, ""));
    TEST_ASSERT_FALSE(anyPublishedTopic(fx.mqtt, stateTopic));
}

void test_ha_discovery_bridge_publishes_both_entities_for_a_multi_channel_device() {
    Fixture fx;
    fx.connectMqtt();

    DeviceCreateResult bus = fx.registry.create(makeBusCreateRequest("I2C Bus"), 10);
    TEST_ASSERT_TRUE_MESSAGE(bus.ok(), bus.validation.message);
    DeviceCreateResult sensor = fx.registry.create(makeHtu21CreateRequest("Greenhouse", bus.deviceId), 10);
    TEST_ASSERT_TRUE_MESSAGE(sensor.ok(), sensor.validation.message);
    fx.dispatcher.tickFastLoop(11);

    TEST_ASSERT_TRUE(saveHaDeviceSettings(fx.haSettingsStore, sensor.deviceId, true, "").ok());
    fx.mqtt.clearPublishedMessages();
    fx.bridge.refreshDevice(sensor.deviceId);

    const std::string temperatureDiscoveryTopic = "homeassistant/sensor/node1/node1_htu21_" + std::to_string(sensor.deviceId) + "/config";
    const std::string humidityDiscoveryTopic =
        "homeassistant/sensor/node1/node1_htu21_humidity_" + std::to_string(sensor.deviceId) + "/config";
    TEST_ASSERT_TRUE_MESSAGE(anyPublishedTopic(fx.mqtt, temperatureDiscoveryTopic), "expected a temperature discovery publish");
    TEST_ASSERT_TRUE_MESSAGE(anyPublishedTopic(fx.mqtt, humidityDiscoveryTopic), "expected a humidity discovery publish");
}
