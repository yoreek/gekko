#include "config/ConfigStore.h"
#include "config/MemoryConfigStorage.h"
#include "core/Clock.h"
#include "core/StateMachine.h"
#include "platform/ArduinoOtaService.h"
#include "portal/PortalServer.h"
#include "wifi/WifiManager.h"

#include <cstring>
#include <unity.h>

using namespace ewfm;

class FakeWifiDriver final : public IWifiDriver {
public:
    bool begin() override {
        ++beginCalls;
        networkStackReadyValue = beginResult;
        if (networkStackReadyValue) {
            setupApActiveValue = false;
        }
        return beginResult;
    }
    bool beginStation(const WiFiCredentials& credentials) override {
        lastCredentials = credentials;
        ++beginStationCalls;
        networkStackReadyValue = true;
        setupApActiveValue = false;
        statusValue = WifiDriverStatus::Connecting;
        return true;
    }
    void disconnect() override {
        ++disconnectCalls;
    }
    void clearStationCredentials() override {
        ++clearStationCredentialsCalls;
        statusValue = WifiDriverStatus::Idle;
        stationIpValue.clear();
    }
    bool startSetupAp(const std::string& ssid, const std::string& password) override {
        (void)password;
        setupApSsid = ssid;
        ++startApCalls;
        networkStackReadyValue = true;
        setupApActiveValue = startApResult;
        return startApResult;
    }
    void stopSetupAp() override {
        ++stopApCalls;
        setupApActiveValue = false;
    }
    WifiDriverStatus status() const override {
        return statusValue;
    }
    bool setupApActive() const {
        return setupApActiveValue;
    }
    bool networkStackReady() const override {
        return networkStackReadyValue;
    }
    bool stationReady() const override {
        return networkStackReadyValue && statusValue == WifiDriverStatus::Connected && ipValid(stationIpValue);
    }
    bool setupApReady() const override {
        return networkStackReadyValue && setupApActiveValue && ipValid(setupApIpValue);
    }
    std::string stationIp() const override {
        return stationIpValue;
    }
    std::string setupApIp() const override {
        return setupApIpValue;
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

    static bool ipValid(const std::string& ip) {
        return !ip.empty() && ip != "0.0.0.0";
    }

    WifiDriverStatus statusValue{WifiDriverStatus::Idle};
    WiFiCredentials lastCredentials;
    std::string setupApSsid;
    int beginCalls{0};
    int beginStationCalls{0};
    int disconnectCalls{0};
    int clearStationCredentialsCalls{0};
    int startApCalls{0};
    int stopApCalls{0};
    bool beginResult{true};
    bool startApResult{true};
    bool networkStackReadyValue{false};
    bool setupApActiveValue{false};
    std::string stationIpValue;
    std::string setupApIpValue{"192.168.4.1"};
};

#undef SM_CLASS
#define SM_CLASS TestMachine
class TestMachine : public StateMachine {
public:
    TestMachine() : StateMachine((PState)&TestMachine::Idle) {}

    void requestRun() {
        runRequested = true;
    }

    State Idle() {
        if (runRequested) {
            SM_GOTO(Running);
        }
    }
    State Running() {
        if (isTimeout(10)) {
            SM_GOTO(TimedOut);
        }
    }
    State TimedOut() {}

    bool runRequested{false};
};
#undef SM_CLASS

void test_no_credentials_enters_setup_ap() {
    ManualClock clock;
    FakeWifiDriver driver;
    WifiManager manager(driver);
    DeviceConfig config = defaultConfig();

    manager.begin(config);
    manager.tick(clock.millis());

    TEST_ASSERT_EQUAL(1, driver.beginCalls);
    TEST_ASSERT_TRUE(manager.apMode());
    TEST_ASSERT_EQUAL(0, driver.startApCalls);

    manager.tick(clock.millis() + 1);
    TEST_ASSERT_EQUAL(1, driver.startApCalls);
    TEST_ASSERT_TRUE(driver.setupApActive());
    TEST_ASSERT_TRUE(driver.setupApSsid.find("ABC123") != std::string::npos);
}

void test_credentials_start_station_connect() {
    ManualClock clock;
    FakeWifiDriver driver;
    WifiManager manager(driver);
    DeviceConfig config = defaultConfig();
    config.wifi.ssid = "office";
    config.wifi.password = "secret";

    manager.begin(config);
    manager.tick(clock.millis());

    TEST_ASSERT_TRUE(manager.connecting());
    TEST_ASSERT_EQUAL(0, driver.beginStationCalls);

    manager.tick(clock.millis());
    TEST_ASSERT_TRUE(manager.checkConnection());
    TEST_ASSERT_EQUAL(1, driver.beginStationCalls);
}

void test_connection_success_keeps_setup_ap() {
    ManualClock clock;
    FakeWifiDriver driver;
    WifiManager manager(driver);
    DeviceConfig config = defaultConfig();
    config.wifi.ssid = "office";

    manager.begin(config);
    manager.tick(clock.millis());
    manager.tick(clock.millis());
    driver.statusValue = WifiDriverStatus::Connected;
    manager.tick(clock.millis());

    TEST_ASSERT_TRUE(manager.connected());
    TEST_ASSERT_EQUAL(0, driver.stopApCalls);
}

void test_failed_connection_retries_and_falls_back() {
    ManualClock clock;
    FakeWifiDriver driver;
    WifiManager manager(driver);
    DeviceConfig config = defaultConfig();
    config.wifi.ssid = "office";
    config.wifiRuntime.connectTimeoutMs = 10;
    config.wifiRuntime.maxConnectRetries = 1;
    config.wifiRuntime.retryDelayMs = 10;

    manager.begin(config);
    manager.tick(clock.millis());
    manager.tick(clock.millis());
    clock.advance(11);
    manager.tick(clock.millis());
    TEST_ASSERT_EQUAL(1, manager.retryCount());
    TEST_ASSERT_TRUE(manager.apMode());

    manager.tick(clock.millis());
    TEST_ASSERT_EQUAL(1, driver.startApCalls);
    TEST_ASSERT_TRUE(manager.apMode());
}

void test_connection_timeout_retry_resets_state_timer() {
    ManualClock clock;
    FakeWifiDriver driver;
    WifiManager manager(driver);
    DeviceConfig config = defaultConfig();
    config.wifi.ssid = "office";
    config.wifiRuntime.connectTimeoutMs = 10;
    config.wifiRuntime.retryDelayMs = 1;
    config.wifiRuntime.maxConnectRetries = 3;

    manager.begin(config);
    manager.tick(clock.millis());
    manager.tick(clock.millis());
    TEST_ASSERT_EQUAL(1, driver.beginStationCalls);

    clock.advance(11);
    manager.tick(clock.millis());
    TEST_ASSERT_TRUE(manager.retryCount() >= 1);
    TEST_ASSERT_FALSE(manager.checkConnection());
    TEST_ASSERT_EQUAL(1, driver.beginStationCalls);

    clock.advance(1);
    manager.tick(clock.millis());
    TEST_ASSERT_EQUAL(1, driver.beginStationCalls);

    clock.advance(1);
    manager.tick(clock.millis());
    TEST_ASSERT_EQUAL(1, manager.retryCount());

    clock.advance(1);
    manager.tick(clock.millis());
    TEST_ASSERT_EQUAL(1, driver.beginStationCalls);

    clock.advance(1);
    manager.tick(clock.millis());
    TEST_ASSERT_EQUAL(2, driver.beginStationCalls);

    clock.advance(1);
    manager.tick(clock.millis());
    TEST_ASSERT_EQUAL(2, driver.beginStationCalls);
}

void test_wifi_readiness_requires_driver_begin_and_valid_addresses() {
    ManualClock clock;
    FakeWifiDriver driver;
    WifiManager manager(driver);
    DeviceConfig config = defaultConfig();
    config.wifi.ssid = "office";
    config.wifi.password = "secret";

    TEST_ASSERT_FALSE(manager.networkStackReady());

    manager.begin(config);
    TEST_ASSERT_TRUE(manager.networkStackReady());
    TEST_ASSERT_FALSE(manager.stationReady());
    TEST_ASSERT_FALSE(manager.setupApReady());

    manager.tick(100);
    manager.tick(101);
    TEST_ASSERT_FALSE(manager.connected());

    driver.statusValue = WifiDriverStatus::Connected;
    manager.tick(102);
    TEST_ASSERT_TRUE(manager.connected());

    manager.tick(103);
    TEST_ASSERT_TRUE(manager.connected());
    TEST_ASSERT_FALSE(manager.stationReady());

    driver.stationIpValue = "192.168.1.240";
    TEST_ASSERT_TRUE(manager.stationReady());
}

void test_setup_ap_readiness_requires_successful_ap_start_and_valid_ip() {
    ManualClock clock;
    FakeWifiDriver driver;
    WifiManager manager(driver);
    DeviceConfig config = defaultConfig();

    manager.begin(config);
    manager.tick(100);
    manager.tick(101);

    TEST_ASSERT_TRUE(manager.apMode());
    TEST_ASSERT_TRUE(driver.setupApActive());
    TEST_ASSERT_TRUE(manager.setupApReady());

    driver.setupApIpValue = "0.0.0.0";
    TEST_ASSERT_FALSE(manager.setupApReady());
}

void test_failed_setup_ap_start_does_not_report_setup_ap_ready() {
    ManualClock clock;
    FakeWifiDriver driver;
    driver.startApResult = false;
    WifiManager manager(driver);
    DeviceConfig config = defaultConfig();

    manager.begin(config);
    manager.tick(100);
    manager.tick(101);

    TEST_ASSERT_TRUE(manager.apMode());
    TEST_ASSERT_EQUAL(1, driver.startApCalls);
    TEST_ASSERT_FALSE(driver.setupApActive());
    TEST_ASSERT_FALSE(manager.setupApReady());
}

void test_station_connect_clears_setup_ap_readiness() {
    ManualClock clock;
    FakeWifiDriver driver;
    WifiManager manager(driver);
    DeviceConfig config = defaultConfig();

    manager.begin(config);
    manager.tick(100);
    manager.tick(101);
    TEST_ASSERT_TRUE(manager.setupApReady());

    WiFiCredentials credentials;
    credentials.ssid = "office";
    credentials.password = "secret";
    TEST_ASSERT_EQUAL(static_cast<int>(WifiManagerResult::Accepted), static_cast<int>(manager.submitCredentials(credentials)));

    TEST_ASSERT_TRUE(manager.apMode());
    TEST_ASSERT_TRUE(driver.setupApActive());
    TEST_ASSERT_TRUE(manager.setupApReady());

    manager.tick(102);
    TEST_ASSERT_FALSE(manager.apMode());
    TEST_ASSERT_EQUAL(0, driver.beginStationCalls);
    TEST_ASSERT_TRUE(driver.setupApActive());
    TEST_ASSERT_FALSE(manager.setupApReady());

    manager.tick(103);
    TEST_ASSERT_FALSE(manager.apMode());
    TEST_ASSERT_TRUE(driver.setupApActive());
    TEST_ASSERT_FALSE(manager.setupApReady());

    manager.tick(104);
    TEST_ASSERT_TRUE(manager.connecting());
    TEST_ASSERT_EQUAL(0, driver.beginStationCalls);
    TEST_ASSERT_TRUE(driver.setupApActive());
    TEST_ASSERT_FALSE(manager.setupApReady());

    manager.tick(105);
    TEST_ASSERT_EQUAL(1, driver.beginStationCalls);
    TEST_ASSERT_FALSE(driver.setupApActive());
    TEST_ASSERT_FALSE(manager.setupApReady());

    driver.statusValue = WifiDriverStatus::Connected;
    driver.stationIpValue = "192.168.1.240";
    manager.tick(106);
    TEST_ASSERT_FALSE(driver.setupApActive());
    TEST_ASSERT_FALSE(manager.setupApReady());
}

void test_wifi_manager_rejects_oversized_credentials() {
    ManualClock clock;
    FakeWifiDriver driver;
    MemoryConfigStorage storage;
    ConfigStore store(storage);
    TEST_ASSERT_TRUE(store.begin());
    TEST_ASSERT_TRUE(store.load().ok());

    WifiManager manager(driver, &store);
    manager.begin(store.config());

    WiFiCredentials credentials;
    credentials.ssid.assign(kMaxSsidLength + 1, 'x');
    WifiManagerResult result = manager.submitCredentials(credentials);

    TEST_ASSERT_EQUAL(static_cast<int>(WifiManagerResult::InvalidInput), static_cast<int>(result));
    TEST_ASSERT_FALSE(store.config().wifi.hasCredentials());
    TEST_ASSERT_EQUAL(0, driver.beginStationCalls);
}

void test_wifi_manager_submit_credentials_saves_and_connects() {
    ManualClock clock;
    FakeWifiDriver driver;
    MemoryConfigStorage storage;
    ConfigStore store(storage);
    TEST_ASSERT_TRUE(store.begin());
    TEST_ASSERT_TRUE(store.load().ok());

    WifiManager manager(driver, &store);
    manager.begin(store.config());

    WiFiCredentials credentials;
    credentials.ssid = "office";
    credentials.password = "secret";
    TEST_ASSERT_EQUAL(static_cast<int>(WifiManagerResult::Accepted), static_cast<int>(manager.submitCredentials(credentials)));

    TEST_ASSERT_FALSE(store.config().wifi.hasCredentials());
    TEST_ASSERT_EQUAL(0, driver.disconnectCalls);
    TEST_ASSERT_EQUAL(0, driver.beginStationCalls);

    manager.tick(clock.millis());
    TEST_ASSERT_FALSE(store.config().wifi.hasCredentials());
    TEST_ASSERT_EQUAL(0, driver.disconnectCalls);
    TEST_ASSERT_EQUAL(0, driver.beginStationCalls);
    TEST_ASSERT_TRUE(manager.apMode());

    manager.tick(clock.millis() + 1);
    TEST_ASSERT_FALSE(store.config().wifi.hasCredentials());
    TEST_ASSERT_EQUAL(0, driver.disconnectCalls);
    TEST_ASSERT_EQUAL(0, driver.beginStationCalls);
    TEST_ASSERT_FALSE(manager.apMode());

    manager.tick(clock.millis() + 2);
    TEST_ASSERT_TRUE(store.config().wifi.hasCredentials());
    TEST_ASSERT_EQUAL_STRING("office", store.config().wifi.ssid.c_str());
    TEST_ASSERT_EQUAL_STRING("secret", store.config().wifi.password.c_str());
    TEST_ASSERT_EQUAL(0, driver.disconnectCalls);
    TEST_ASSERT_FALSE(manager.connecting());
    TEST_ASSERT_FALSE(manager.apMode());

    manager.tick(clock.millis() + 3);
    TEST_ASSERT_TRUE(manager.connecting());
    TEST_ASSERT_EQUAL(0, driver.beginStationCalls);

    manager.tick(clock.millis() + 4);
    TEST_ASSERT_EQUAL(1, driver.beginStationCalls);
}

void test_wifi_manager_rejects_ble_config_when_disabled() {
    ManualClock clock;
    FakeWifiDriver driver;
    WifiManager manager(driver);
    DeviceConfig config = defaultConfig();
    config.provisioning.mobileProvisioningEnabled = false;

    manager.begin(config);

    TEST_ASSERT_FALSE(manager.requestBleConfig());
    manager.tick(clock.millis());
    TEST_ASSERT_FALSE(manager.bleConfigMode());
    TEST_ASSERT_EQUAL(0, manager.bleStartCount());
}

void test_portal_waits_for_network_stack_before_http_start() {
    ManualClock clock;
    FakeWifiDriver driver;
    MemoryConfigStorage storage;
    ConfigStore store(storage);
    TEST_ASSERT_TRUE(store.begin());
    TEST_ASSERT_TRUE(store.load().ok());

    WifiManager manager(driver);
    PortalServer portal(manager, driver);

    TEST_ASSERT_TRUE(portal.begin());
    portal.tick(100);
    TEST_ASSERT_EQUAL(static_cast<int>(PortalRuntimeState::WaitingForNetwork), static_cast<int>(portal.state()));
    TEST_ASSERT_FALSE(portal.httpRunning());
    TEST_ASSERT_EQUAL(0, portal.httpStartCount());

    driver.networkStackReadyValue = true;
    portal.tick(101);
    TEST_ASSERT_FALSE(portal.httpRunning());

    portal.tick(102);
    TEST_ASSERT_TRUE(portal.httpRunning());
    TEST_ASSERT_EQUAL(static_cast<int>(PortalRuntimeState::Running), static_cast<int>(portal.state()));
    TEST_ASSERT_EQUAL(1, portal.httpStartCount());
}

void test_portal_dns_follows_setup_ap_readiness_without_http_restart() {
    ManualClock clock;
    FakeWifiDriver driver;
    driver.networkStackReadyValue = true;
    MemoryConfigStorage storage;
    ConfigStore store(storage);
    TEST_ASSERT_TRUE(store.begin());
    TEST_ASSERT_TRUE(store.load().ok());

    WifiManager manager(driver);
    PortalServer portal(manager, driver);

    TEST_ASSERT_TRUE(portal.begin());
    portal.tick(100);
    portal.tick(101);
    portal.tick(102);
    TEST_ASSERT_TRUE(portal.httpRunning());
    TEST_ASSERT_FALSE(portal.dnsRunning());

    driver.setupApActiveValue = true;
    driver.setupApIpValue = "0.0.0.0";
    portal.tick(103);
    TEST_ASSERT_FALSE(portal.dnsRunning());

    driver.setupApIpValue = "192.168.4.1";
    portal.tick(104);
    TEST_ASSERT_TRUE(portal.dnsRunning());
    TEST_ASSERT_EQUAL(1, portal.dnsStartCount());
    TEST_ASSERT_EQUAL(1, portal.httpStartCount());

    driver.setupApActiveValue = false;
    portal.tick(105);
    TEST_ASSERT_FALSE(portal.dnsRunning());
    TEST_ASSERT_EQUAL(1, portal.dnsStopCount());
    TEST_ASSERT_EQUAL(1, portal.httpStartCount());
}

void test_portal_restarts_http_only_when_network_stack_is_lost() {
    ManualClock clock;
    FakeWifiDriver driver;
    driver.networkStackReadyValue = true;
    MemoryConfigStorage storage;
    ConfigStore store(storage);
    TEST_ASSERT_TRUE(store.begin());
    TEST_ASSERT_TRUE(store.load().ok());

    WifiManager manager(driver);
    PortalServer portal(manager, driver);

    TEST_ASSERT_TRUE(portal.begin());
    portal.tick(100);
    portal.tick(101);
    portal.tick(102);
    TEST_ASSERT_TRUE(portal.httpRunning());
    TEST_ASSERT_EQUAL(1, portal.httpStartCount());

    driver.setupApActiveValue = true;
    portal.tick(103);
    driver.setupApActiveValue = false;
    portal.tick(104);
    TEST_ASSERT_TRUE(portal.httpRunning());
    TEST_ASSERT_EQUAL(1, portal.httpStartCount());
    TEST_ASSERT_EQUAL(0, portal.httpStopCount());

    driver.networkStackReadyValue = false;
    portal.tick(105);
    TEST_ASSERT_FALSE(portal.httpRunning());
    TEST_ASSERT_EQUAL(1, portal.httpStopCount());

    driver.networkStackReadyValue = true;
    portal.tick(106);
    portal.tick(107);
    TEST_ASSERT_TRUE(portal.httpRunning());
    TEST_ASSERT_EQUAL(2, portal.httpStartCount());
}

void test_wifi_manager_ble_config_request_starts_ble_mode() {
    ManualClock clock;
    FakeWifiDriver driver;
    WifiManager manager(driver);
    DeviceConfig config = defaultConfig();

    manager.begin(config);

    TEST_ASSERT_TRUE(manager.requestBleConfig());
    TEST_ASSERT_FALSE(manager.bleConfigMode());
    TEST_ASSERT_FALSE(manager.bleConfigRunning());
    TEST_ASSERT_EQUAL(0, manager.bleStartCount());

    manager.tick(clock.millis());
    TEST_ASSERT_TRUE(manager.apMode());

    manager.tick(clock.millis() + 1);
    TEST_ASSERT_TRUE(manager.bleConfigMode());
    TEST_ASSERT_FALSE(manager.bleConfigRunning());
    TEST_ASSERT_EQUAL(0, manager.bleStartCount());

    manager.tick(clock.millis() + 2);
    TEST_ASSERT_TRUE(manager.bleConfigRunning());
    TEST_ASSERT_EQUAL(1, manager.bleStartCount());
}

void test_wifi_manager_rejects_portal_submit_while_ble_config_is_running() {
    FakeWifiDriver driver;
    WifiManager manager(driver);
    DeviceConfig config = defaultConfig();

    manager.begin(config);
    TEST_ASSERT_TRUE(manager.requestBleConfig());
    manager.tick(100);
    manager.tick(101);
    manager.tick(102);
    TEST_ASSERT_TRUE(manager.bleConfigRunning());

    WiFiCredentials credentials;
    credentials.ssid = "office";
    credentials.password = "secret";
    TEST_ASSERT_EQUAL(static_cast<int>(WifiManagerResult::Busy), static_cast<int>(manager.submitCredentials(credentials)));
    TEST_ASSERT_EQUAL(0, driver.beginStationCalls);
}

void test_wifi_manager_ble_timeout_stops_deinitializes_and_returns() {
    FakeWifiDriver driver;
    MemoryConfigStorage storage;
    ConfigStore store(storage);
    TEST_ASSERT_TRUE(store.begin());
    TEST_ASSERT_TRUE(store.load().ok());

    DeviceConfig config = store.config();
    config.provisioning.sessionTimeoutMs = 10;
    TEST_ASSERT_TRUE(store.save(config).ok());

    WifiManager manager(driver, &store);
    manager.begin(store.config());

    TEST_ASSERT_TRUE(manager.requestBleConfig());
    manager.tick(100);
    manager.tick(101);
    manager.tick(102);
    TEST_ASSERT_TRUE(manager.bleConfigRunning());
    TEST_ASSERT_EQUAL(1, manager.bleStartCount());

    manager.tick(113);
    TEST_ASSERT_TRUE(manager.bleConfigMode());
    TEST_ASSERT_EQUAL(0, manager.bleStopCount());

    manager.tick(114);
    TEST_ASSERT_EQUAL(1, manager.bleStopCount());
    TEST_ASSERT_EQUAL(0, manager.bleDeinitCount());

    manager.tick(115);
    TEST_ASSERT_EQUAL(1, manager.bleStopCount());
    TEST_ASSERT_EQUAL(0, manager.bleDeinitCount());

    manager.tick(116);
    TEST_ASSERT_EQUAL(1, manager.bleDeinitCount());

    manager.tick(117);
    TEST_ASSERT_FALSE(manager.bleConfigMode());
    TEST_ASSERT_FALSE(store.config().wifi.hasCredentials());
    TEST_ASSERT_EQUAL(0, driver.beginStationCalls);
}

void test_wifi_manager_ble_stop_wait_is_bounded_when_prov_end_is_missing() {
    FakeWifiDriver driver;
    MemoryConfigStorage storage;
    ConfigStore store(storage);
    TEST_ASSERT_TRUE(store.begin());
    TEST_ASSERT_TRUE(store.load().ok());

    DeviceConfig config = store.config();
    config.provisioning.sessionTimeoutMs = 10;
    TEST_ASSERT_TRUE(store.save(config).ok());

    WifiManager manager(driver, &store);
    manager.setBleStopAutoEnd(false);
    manager.begin(store.config());

    TEST_ASSERT_TRUE(manager.requestBleConfig());
    manager.tick(100);
    manager.tick(101);
    manager.tick(102);
    manager.tick(113);
    manager.tick(114);
    TEST_ASSERT_EQUAL(1, manager.bleStopCount());
    TEST_ASSERT_EQUAL(0, manager.bleDeinitCount());

    manager.tick(5115);
    TEST_ASSERT_EQUAL(0, manager.bleDeinitCount());

    manager.tick(5116);
    TEST_ASSERT_EQUAL(1, manager.bleDeinitCount());

    manager.tick(5117);
    TEST_ASSERT_EQUAL(1, manager.bleDeinitCount());

    manager.tick(5118);
    TEST_ASSERT_FALSE(manager.bleConfigMode());
}

void test_wifi_manager_ble_credentials_stop_deinit_save_and_connect() {
    FakeWifiDriver driver;
    MemoryConfigStorage storage;
    ConfigStore store(storage);
    TEST_ASSERT_TRUE(store.begin());
    TEST_ASSERT_TRUE(store.load().ok());

    WifiManager manager(driver, &store);
    manager.begin(store.config());

    TEST_ASSERT_TRUE(manager.requestBleConfig());
    manager.tick(100);
    manager.tick(101);
    manager.tick(102);
    TEST_ASSERT_TRUE(manager.bleConfigRunning());
    TEST_ASSERT_EQUAL(1, manager.bleStartCount());

    manager.simulateBleCredentialsReceived("office", "secret");
    manager.tick(103);
    TEST_ASSERT_TRUE(manager.bleConfigMode());
    TEST_ASSERT_EQUAL(0, manager.bleStopCount());

    manager.tick(104);
    TEST_ASSERT_EQUAL(0, manager.bleStopCount());
    TEST_ASSERT_EQUAL(0, manager.bleDeinitCount());

    manager.tick(105);
    TEST_ASSERT_EQUAL(1, manager.bleStopCount());
    TEST_ASSERT_EQUAL(0, manager.bleDeinitCount());

    manager.tick(106);
    TEST_ASSERT_EQUAL(0, manager.bleDeinitCount());

    manager.tick(107);
    TEST_ASSERT_EQUAL(1, manager.bleDeinitCount());
    TEST_ASSERT_TRUE(store.config().wifi.hasCredentials());

    manager.tick(108);
    TEST_ASSERT_TRUE(store.config().wifi.hasCredentials());
    TEST_ASSERT_EQUAL_STRING("office", store.config().wifi.ssid.c_str());
    TEST_ASSERT_EQUAL_STRING("secret", store.config().wifi.password.c_str());
    TEST_ASSERT_TRUE(manager.connecting());
    TEST_ASSERT_EQUAL(0, driver.beginStationCalls);

    manager.tick(109);
    TEST_ASSERT_EQUAL(1, driver.beginStationCalls);
    TEST_ASSERT_EQUAL_STRING("office", driver.lastCredentials.ssid.c_str());
    TEST_ASSERT_FALSE(manager.connecting());
}

void test_wifi_manager_invalid_ble_credentials_do_not_start_station_connect() {
    FakeWifiDriver driver;
    MemoryConfigStorage storage;
    ConfigStore store(storage);
    TEST_ASSERT_TRUE(store.begin());
    TEST_ASSERT_TRUE(store.load().ok());

    WifiManager manager(driver, &store);
    manager.begin(store.config());

    TEST_ASSERT_TRUE(manager.requestBleConfig());
    manager.tick(100);
    manager.tick(101);
    manager.tick(102);
    TEST_ASSERT_TRUE(manager.bleConfigRunning());

    manager.simulateBleCredentialsReceived("", "secret");
    manager.tick(103);
    manager.tick(104);
    manager.tick(105);
    manager.tick(106);
    TEST_ASSERT_EQUAL(0, manager.bleDeinitCount());

    manager.tick(107);
    TEST_ASSERT_EQUAL(1, manager.bleDeinitCount());
    TEST_ASSERT_FALSE(store.config().wifi.hasCredentials());
    TEST_ASSERT_EQUAL(0, driver.beginStationCalls);

    manager.tick(108);
    TEST_ASSERT_TRUE(manager.apMode());
    TEST_ASSERT_EQUAL(0, driver.beginStationCalls);
}

void test_arduino_ota_starts_after_wifi_connects() {
    ManualClock clock;
    FakeWifiDriver driver;
    WifiManager manager(driver);
    DeviceConfig config = defaultConfig();
    config.wifi.ssid = "office";
    config.wifi.password = "secret";

    manager.begin(config);
    ArduinoOtaService ota;
    ota.begin(config.deviceName, manager);

    ota.tick(100);
    TEST_ASSERT_FALSE(ota.started());

    manager.tick(100);
    ota.tick(100);
    TEST_ASSERT_FALSE(ota.started());

    manager.tick(101);
    ota.tick(101);
    TEST_ASSERT_FALSE(ota.started());

    driver.statusValue = WifiDriverStatus::Connected;
    driver.stationIpValue = "192.168.1.240";
    manager.tick(102);
    ota.tick(102);
    TEST_ASSERT_FALSE(ota.started());

    ota.tick(103);
    TEST_ASSERT_TRUE(ota.started());
    TEST_ASSERT_TRUE(ota.running());
    TEST_ASSERT_EQUAL(1, ota.startCount());
}

void test_arduino_ota_starts_on_setup_ap_readiness() {
    ManualClock clock;
    FakeWifiDriver driver;
    WifiManager manager(driver);
    DeviceConfig config = defaultConfig();

    manager.begin(config);
    ArduinoOtaService ota;
    ota.begin(config.deviceName, manager);

    manager.tick(100);
    ota.tick(100);
    TEST_ASSERT_FALSE(ota.started());

    manager.tick(101);
    ota.tick(101);
    TEST_ASSERT_FALSE(ota.started());

    ota.tick(102);
    TEST_ASSERT_TRUE(ota.started());
    TEST_ASSERT_TRUE(ota.running());
    TEST_ASSERT_EQUAL(1, ota.startCount());
}

void test_arduino_ota_stops_and_restarts_after_wifi_loss() {
    ManualClock clock;
    FakeWifiDriver driver;
    WifiManager manager(driver);
    DeviceConfig config = defaultConfig();
    config.wifi.ssid = "office";
    config.wifi.password = "secret";

    manager.begin(config);
    manager.tick(100);
    manager.tick(101);
    manager.tick(102);
    driver.statusValue = WifiDriverStatus::Connected;
    driver.stationIpValue = "192.168.1.240";
    manager.tick(103);

    ArduinoOtaService ota;
    ota.begin(config.deviceName, manager);
    ota.tick(104);
    ota.tick(105);
    TEST_ASSERT_FALSE(ota.started());

    ota.tick(106);
    TEST_ASSERT_TRUE(ota.started());
    TEST_ASSERT_EQUAL(1, ota.startCount());

    driver.statusValue = WifiDriverStatus::Disconnected;
    driver.stationIpValue.clear();
    manager.tick(107);
    ota.tick(107);

    TEST_ASSERT_FALSE(ota.started());
    TEST_ASSERT_TRUE(ota.waitingForStation());
    TEST_ASSERT_EQUAL(1, ota.stopCount());

    manager.tick(108);
    ota.tick(108);
    TEST_ASSERT_FALSE(ota.started());

    manager.tick(109);
    ota.tick(109);
    TEST_ASSERT_FALSE(ota.started());

    driver.statusValue = WifiDriverStatus::Connected;
    driver.stationIpValue = "192.168.1.241";
    ota.tick(110);
    TEST_ASSERT_FALSE(ota.started());

    manager.tick(110);
    ota.tick(111);
    TEST_ASSERT_FALSE(ota.started());

    ota.tick(112);

    TEST_ASSERT_TRUE(ota.started());
    TEST_ASSERT_EQUAL(2, ota.startCount());
}

void test_arduino_ota_restarts_after_station_ip_changes() {
    ManualClock clock;
    FakeWifiDriver driver;
    WifiManager manager(driver);
    DeviceConfig config = defaultConfig();
    config.wifi.ssid = "office";
    config.wifi.password = "secret";

    manager.begin(config);
    manager.tick(100);
    manager.tick(101);
    manager.tick(102);
    driver.statusValue = WifiDriverStatus::Connected;
    driver.stationIpValue = "192.168.1.240";
    manager.tick(103);

    ArduinoOtaService ota;
    ota.begin(config.deviceName, manager);
    ota.tick(104);
    ota.tick(105);
    TEST_ASSERT_FALSE(ota.started());

    ota.tick(106);
    TEST_ASSERT_TRUE(ota.started());

    driver.stationIpValue = "192.168.1.241";
    ota.tick(107);

    TEST_ASSERT_FALSE(ota.started());
    TEST_ASSERT_TRUE(ota.waitingForStation());
    TEST_ASSERT_EQUAL(1, ota.stopCount());

    ota.tick(106);
    TEST_ASSERT_FALSE(ota.started());

    ota.tick(107);
    TEST_ASSERT_TRUE(ota.started());
    TEST_ASSERT_EQUAL(2, ota.startCount());
}

void test_state_machine_stack_and_return_to_popped_state() {
    TestMachine machine;
    TEST_ASSERT_TRUE(machine.pushState((StateMachine::PState)&TestMachine::TimedOut));
    machine.transitionTo((StateMachine::PState)&TestMachine::Running, 10);

    TEST_ASSERT_TRUE(machine.returnToPopped(20));
    TEST_ASSERT_TRUE(machine.is((StateMachine::PState)&TestMachine::TimedOut));
    TEST_ASSERT_EQUAL_UINT32(20, machine.stateUpdated());
}

void test_state_machine_pause_restart_and_updated_flag() {
    TestMachine machine;
    machine.transitionTo((StateMachine::PState)&TestMachine::Running, 10);
    TEST_ASSERT_TRUE(machine.isStateUpdated());

    machine.transitionTo((StateMachine::PState)&TestMachine::Running, 20);
    TEST_ASSERT_FALSE(machine.isStateUpdated());

    machine.pause();
    TEST_ASSERT_TRUE(machine.isPaused());
    machine.restart();
    TEST_ASSERT_FALSE(machine.isPaused());
}

void test_state_machine_timeout_helpers() {
    TestMachine machine;

    machine.transitionTo((StateMachine::PState)&TestMachine::Running, 10);
    TEST_ASSERT_FALSE(machine.elapsed(19, 10));
    TEST_ASSERT_TRUE(machine.elapsed(20, 10));

    machine.tick(10);
    machine.requestRun();
    machine.tick(11);
    TEST_ASSERT_TRUE(machine.is((StateMachine::PState)&TestMachine::Running));

    machine.tick(21);
    TEST_ASSERT_TRUE(machine.is((StateMachine::PState)&TestMachine::TimedOut));
    TEST_ASSERT_TRUE(EWFM_SM_TIME_REACHED(5, 5));
}
