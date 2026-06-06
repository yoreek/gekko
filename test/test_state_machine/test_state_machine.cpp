#include "config/ConfigStore.h"
#include "config/MemoryConfigStorage.h"
#include "core/Clock.h"
#include "core/StateMachine.h"
#include "platform/ArduinoOtaService.h"
#include "portal/PortalAssets.h"
#include "portal/PortalServer.h"
#include "provisioning/MobileProvisioning.h"
#include "provisioning/ProvisioningCoordinator.h"
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
    bool prepareProvisioningScan() override {
        ++prepareProvisioningScanCalls;
        networkStackReadyValue = true;
        return prepareProvisioningScanResult;
    }
    WifiDriverStatus status() const override {
        return statusValue;
    }
    bool setupApActive() const override {
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
    int prepareProvisioningScanCalls{0};
    bool beginResult{true};
    bool startApResult{true};
    bool prepareProvisioningScanResult{true};
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
    TEST_ASSERT_EQUAL(1, driver.startApCalls);
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
    config.wifiRuntime.maxConnectRetries = 1;
    config.wifiRuntime.retryDelayMs = 10;

    manager.begin(config);
    manager.tick(clock.millis());
    manager.tick(clock.millis());
    driver.statusValue = WifiDriverStatus::Failed;
    manager.tick(clock.millis());
    TEST_ASSERT_EQUAL(1, manager.retryCount());

    clock.advance(20);
    driver.statusValue = WifiDriverStatus::Failed;
    manager.tick(clock.millis());
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
    TEST_ASSERT_TRUE(manager.setupApReady());

    WiFiCredentials credentials;
    credentials.ssid = "office";
    credentials.password = "secret";
    manager.updateCredentials(credentials);

    TEST_ASSERT_TRUE(manager.apMode());
    TEST_ASSERT_TRUE(driver.setupApActive());
    TEST_ASSERT_TRUE(manager.setupApReady());

    manager.tick(101);
    TEST_ASSERT_TRUE(manager.connecting());
    TEST_ASSERT_TRUE(driver.setupApActive());
    TEST_ASSERT_FALSE(manager.setupApReady());

    manager.tick(102);
    TEST_ASSERT_EQUAL(1, driver.beginStationCalls);
    TEST_ASSERT_FALSE(driver.setupApActive());
    TEST_ASSERT_FALSE(manager.setupApReady());

    driver.statusValue = WifiDriverStatus::Connected;
    driver.stationIpValue = "192.168.1.240";
    manager.tick(103);
    TEST_ASSERT_FALSE(driver.setupApActive());
    TEST_ASSERT_FALSE(manager.setupApReady());
}

void test_provisioning_coordinator_rejects_oversized_http_credentials() {
    ManualClock clock;
    FakeWifiDriver driver;
    MemoryConfigStorage storage;
    ConfigStore store(storage);
    TEST_ASSERT_TRUE(store.begin());
    TEST_ASSERT_TRUE(store.load().ok());

    WifiManager manager(driver);
    manager.begin(store.config());
    ProvisioningCoordinator coordinator(store, manager);

    WiFiCredentials credentials;
    credentials.ssid.assign(kMaxSsidLength + 1, 'x');
    ProvisioningResult result = coordinator.submitWifiCredentials(credentials);

    TEST_ASSERT_EQUAL(static_cast<int>(ProvisioningResult::InvalidInput), static_cast<int>(result));
    TEST_ASSERT_EQUAL(0, driver.beginStationCalls);
}

void test_provisioning_coordinator_reset_clears_credentials_and_starts_softap() {
    ManualClock clock;
    FakeWifiDriver driver;
    MemoryConfigStorage storage;
    ConfigStore store(storage);
    TEST_ASSERT_TRUE(store.begin());
    TEST_ASSERT_TRUE(store.load().ok());

    WifiManager manager(driver);
    manager.begin(store.config());
    ProvisioningCoordinator coordinator(store, manager);

    WiFiCredentials credentials;
    credentials.ssid = "office";
    credentials.password = "secret";
    TEST_ASSERT_EQUAL(static_cast<int>(ProvisioningResult::Accepted), static_cast<int>(coordinator.submitWifiCredentials(credentials)));

    coordinator.resetWifiCredentials();

    TEST_ASSERT_FALSE(store.config().wifi.hasCredentials());
    TEST_ASSERT_FALSE(manager.credentials().hasCredentials());
    manager.tick(clock.millis());
    TEST_ASSERT_TRUE(manager.apMode());
    TEST_ASSERT_EQUAL(0, driver.clearStationCredentialsCalls);
    TEST_ASSERT_EQUAL(0, driver.disconnectCalls);
    TEST_ASSERT_EQUAL(1, driver.startApCalls);
}

void test_provisioning_coordinator_queues_mobile_reentry_request() {
    ManualClock clock;
    FakeWifiDriver driver;
    MemoryConfigStorage storage;
    ConfigStore store(storage);
    TEST_ASSERT_TRUE(store.begin());
    TEST_ASSERT_TRUE(store.load().ok());

    WifiManager manager(driver);
    manager.begin(store.config());
    ProvisioningCoordinator coordinator(store, manager);

    TEST_ASSERT_TRUE(coordinator.requestMobileProvisioningReentry());
    TEST_ASSERT_TRUE(coordinator.takeMobileProvisioningReentryRequest());
    TEST_ASSERT_FALSE(coordinator.takeMobileProvisioningReentryRequest());
}

void test_portal_waits_for_network_stack_before_http_start() {
    ManualClock clock;
    FakeWifiDriver driver;
    MemoryConfigStorage storage;
    ConfigStore store(storage);
    TEST_ASSERT_TRUE(store.begin());
    TEST_ASSERT_TRUE(store.load().ok());

    WifiManager manager(driver);
    ProvisioningCoordinator coordinator(store, manager);
    PortalServer portal(coordinator, driver);

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
    ProvisioningCoordinator coordinator(store, manager);
    PortalServer portal(coordinator, driver);

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
    ProvisioningCoordinator coordinator(store, manager);
    PortalServer portal(coordinator, driver);

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

void test_mobile_provisioning_starts_when_credentials_are_missing() {
    ManualClock clock;
    FakeWifiDriver driver;
    MemoryConfigStorage storage;
    ConfigStore store(storage);
    TEST_ASSERT_TRUE(store.begin());
    TEST_ASSERT_TRUE(store.load().ok());

    WifiManager manager(driver);
    manager.begin(store.config());
    ProvisioningCoordinator coordinator(store, manager);
    MobileProvisioning provisioning(coordinator, manager);
    provisioning.begin(store.config());

    provisioning.tick(100);

    TEST_ASSERT_TRUE(provisioning.running());
}

void test_mobile_provisioning_starts_when_wifi_enters_setup_ap() {
    ManualClock clock;
    FakeWifiDriver driver;
    MemoryConfigStorage storage;
    ConfigStore store(storage);
    TEST_ASSERT_TRUE(store.begin());
    TEST_ASSERT_TRUE(store.load().ok());

    DeviceConfig config = store.config();
    config.wifi.ssid = "office";
    config.wifi.password = "secret";
    config.wifiRuntime.maxConnectRetries = 0;
    TEST_ASSERT_TRUE(store.save(config).ok());

    WifiManager manager(driver);
    manager.begin(store.config());
    ProvisioningCoordinator coordinator(store, manager);
    MobileProvisioning provisioning(coordinator, manager);
    provisioning.begin(store.config());

    manager.tick(100);
    provisioning.tick(100);
    TEST_ASSERT_FALSE(provisioning.running());

    manager.tick(101);
    TEST_ASSERT_FALSE(manager.apMode());

    driver.statusValue = WifiDriverStatus::Failed;
    manager.tick(102);
    TEST_ASSERT_TRUE(manager.apMode());

    provisioning.tick(102);
    TEST_ASSERT_TRUE(provisioning.running());
}

void test_mobile_provisioning_reentry_resets_credentials_and_restarts_ble() {
    ManualClock clock;
    FakeWifiDriver driver;
    MemoryConfigStorage storage;
    ConfigStore store(storage);
    TEST_ASSERT_TRUE(store.begin());
    TEST_ASSERT_TRUE(store.load().ok());

    DeviceConfig config = store.config();
    config.wifi.ssid = "office";
    config.wifi.password = "secret";
    TEST_ASSERT_TRUE(store.save(config).ok());

    WifiManager manager(driver);
    manager.begin(store.config());
    ProvisioningCoordinator coordinator(store, manager);
    MobileProvisioning provisioning(coordinator, manager);
    provisioning.begin(store.config());

    TEST_ASSERT_TRUE(coordinator.requestMobileProvisioningReentry());
    provisioning.tick(200);
    manager.tick(200);

    TEST_ASSERT_FALSE(store.config().wifi.hasCredentials());
    TEST_ASSERT_FALSE(manager.credentials().hasCredentials());
    TEST_ASSERT_TRUE(manager.apMode());
    TEST_ASSERT_TRUE(provisioning.running());
    TEST_ASSERT_EQUAL(0, driver.clearStationCredentialsCalls);
    TEST_ASSERT_EQUAL(1, driver.startApCalls);
}

void test_mobile_provisioning_timeout_uses_supplied_timestamp() {
    ManualClock clock;
    FakeWifiDriver driver;
    MemoryConfigStorage storage;
    ConfigStore store(storage);
    TEST_ASSERT_TRUE(store.begin());
    TEST_ASSERT_TRUE(store.load().ok());

    WifiManager manager(driver);
    manager.begin(store.config());
    ProvisioningCoordinator coordinator(store, manager);
    MobileProvisioning provisioning(coordinator, manager);

    DeviceConfig config = store.config();
    config.wifi.ssid = "office";
    config.provisioning.sessionTimeoutMs = 10;
    TEST_ASSERT_TRUE(store.save(config).ok());

    manager.begin(store.config());
    provisioning.begin(store.config());
    provisioning.start(100);
    provisioning.tick(109);
    TEST_ASSERT_TRUE(provisioning.running());

    provisioning.tick(111);
    TEST_ASSERT_TRUE(provisioning.timedOut());
}

void test_mobile_provisioning_restart_ble_keeps_session_active() {
    ManualClock clock;
    FakeWifiDriver driver;
    MemoryConfigStorage storage;
    ConfigStore store(storage);
    TEST_ASSERT_TRUE(store.begin());
    TEST_ASSERT_TRUE(store.load().ok());

    WifiManager manager(driver);
    manager.begin(store.config());
    ProvisioningCoordinator coordinator(store, manager);
    MobileProvisioning provisioning(coordinator, manager);

    DeviceConfig config = store.config();
    config.provisioning.mobileProvisioningEnabled = true;

    provisioning.begin(config);
    provisioning.start(100);
    TEST_ASSERT_TRUE(provisioning.running());

    provisioning.restartBle(150);
    TEST_ASSERT_FALSE(provisioning.running());
    TEST_ASSERT_TRUE(provisioning.restartPending());

    provisioning.tick(1649);
    TEST_ASSERT_TRUE(provisioning.restartPending());

    provisioning.tick(1650);
    TEST_ASSERT_TRUE(provisioning.running());
}

void test_mobile_provisioning_restart_after_timeout_is_allowed() {
    ManualClock clock;
    FakeWifiDriver driver;
    MemoryConfigStorage storage;
    ConfigStore store(storage);
    TEST_ASSERT_TRUE(store.begin());
    TEST_ASSERT_TRUE(store.load().ok());

    WifiManager manager(driver);
    manager.begin(store.config());
    ProvisioningCoordinator coordinator(store, manager);
    MobileProvisioning provisioning(coordinator, manager);

    DeviceConfig config = store.config();
    config.provisioning.mobileProvisioningEnabled = true;
    config.provisioning.sessionTimeoutMs = 10;

    provisioning.begin(config);
    provisioning.start(100);
    provisioning.tick(111);
    TEST_ASSERT_FALSE(provisioning.running());
    TEST_ASSERT_TRUE(provisioning.restartPending());

    provisioning.tick(1610);
    TEST_ASSERT_TRUE(provisioning.restartPending());

    provisioning.tick(1611);
    TEST_ASSERT_TRUE(provisioning.running());
    TEST_ASSERT_EQUAL_UINT32(1611, provisioning.stateUpdated());
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

    driver.statusValue = WifiDriverStatus::Connected;
    driver.stationIpValue = "192.168.1.241";
    manager.tick(109);
    ota.tick(109);
    TEST_ASSERT_FALSE(ota.started());

    ota.tick(110);

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

void test_portal_html_exposes_provisioning_reentry_action() {
    const char* html = portalHtml();
    TEST_ASSERT_NOT_NULL(strstr(html, "/api/provisioning/reenter"));
    TEST_ASSERT_NOT_NULL(strstr(html, "Re-enter BLE provisioning"));
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

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_no_credentials_enters_setup_ap);
    RUN_TEST(test_credentials_start_station_connect);
    RUN_TEST(test_connection_success_keeps_setup_ap);
    RUN_TEST(test_failed_connection_retries_and_falls_back);
    RUN_TEST(test_connection_timeout_retry_resets_state_timer);
    RUN_TEST(test_wifi_readiness_requires_driver_begin_and_valid_addresses);
    RUN_TEST(test_setup_ap_readiness_requires_successful_ap_start_and_valid_ip);
    RUN_TEST(test_failed_setup_ap_start_does_not_report_setup_ap_ready);
    RUN_TEST(test_station_connect_clears_setup_ap_readiness);
    RUN_TEST(test_provisioning_coordinator_rejects_oversized_http_credentials);
    RUN_TEST(test_provisioning_coordinator_reset_clears_credentials_and_starts_softap);
    RUN_TEST(test_provisioning_coordinator_queues_mobile_reentry_request);
    RUN_TEST(test_portal_waits_for_network_stack_before_http_start);
    RUN_TEST(test_portal_dns_follows_setup_ap_readiness_without_http_restart);
    RUN_TEST(test_portal_restarts_http_only_when_network_stack_is_lost);
    RUN_TEST(test_mobile_provisioning_starts_when_credentials_are_missing);
    RUN_TEST(test_mobile_provisioning_starts_when_wifi_enters_setup_ap);
    RUN_TEST(test_mobile_provisioning_reentry_resets_credentials_and_restarts_ble);
    RUN_TEST(test_mobile_provisioning_timeout_uses_supplied_timestamp);
    RUN_TEST(test_mobile_provisioning_restart_ble_keeps_session_active);
    RUN_TEST(test_mobile_provisioning_restart_after_timeout_is_allowed);
    RUN_TEST(test_arduino_ota_starts_after_wifi_connects);
    RUN_TEST(test_arduino_ota_starts_on_setup_ap_readiness);
    RUN_TEST(test_arduino_ota_stops_and_restarts_after_wifi_loss);
    RUN_TEST(test_arduino_ota_restarts_after_station_ip_changes);
    RUN_TEST(test_portal_html_exposes_provisioning_reentry_action);
    RUN_TEST(test_state_machine_stack_and_return_to_popped_state);
    RUN_TEST(test_state_machine_pause_restart_and_updated_flag);
    RUN_TEST(test_state_machine_timeout_helpers);
    return UNITY_END();
}
