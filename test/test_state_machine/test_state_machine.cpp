#include "config/ConfigStore.h"
#include "config/MemoryConfigStorage.h"
#include "core/Clock.h"
#include "core/StateMachine.h"
#include "portal/PortalAssets.h"
#include "provisioning/MobileProvisioning.h"
#include "provisioning/ProvisioningCoordinator.h"
#include "wifi/WifiManager.h"

#include <cstring>
#include <unity.h>

using namespace ewfm;

class FakeWifiDriver final : public IWifiDriver {
public:
    bool beginStation(const WiFiCredentials& credentials) override {
        lastCredentials = credentials;
        ++beginStationCalls;
        statusValue = WifiDriverStatus::Connecting;
        return true;
    }
    void disconnect() override {
        ++disconnectCalls;
    }
    void clearStationCredentials() override {
        ++clearStationCredentialsCalls;
        statusValue = WifiDriverStatus::Idle;
    }
    bool startSetupAp(const std::string& ssid, const std::string& password) override {
        (void)password;
        setupApSsid = ssid;
        ++startApCalls;
        return true;
    }
    void stopSetupAp() override {
        ++stopApCalls;
    }
    WifiDriverStatus status() const override {
        return statusValue;
    }
    bool setupApActive() const override {
        return setupApActiveValue;
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

    WifiDriverStatus statusValue{WifiDriverStatus::Idle};
    WiFiCredentials lastCredentials;
    std::string setupApSsid;
    int beginStationCalls{0};
    int disconnectCalls{0};
    int clearStationCredentialsCalls{0};
    int startApCalls{0};
    int stopApCalls{0};
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

void test_no_credentials_enters_provisioning_fallback() {
    ManualClock clock;
    FakeWifiDriver driver;
    WifiManager manager(driver, clock);
    DeviceConfig config = defaultConfig();

    manager.begin(config);
    manager.tick(clock.millis());

    TEST_ASSERT_TRUE(manager.provisioningFallback());
    TEST_ASSERT_EQUAL(1, driver.startApCalls);
    TEST_ASSERT_TRUE(driver.setupApSsid.find("ABC123") != std::string::npos);
}

void test_credentials_start_station_connect() {
    ManualClock clock;
    FakeWifiDriver driver;
    WifiManager manager(driver, clock);
    DeviceConfig config = defaultConfig();
    config.wifi.ssid = "office";
    config.wifi.password = "secret";

    manager.begin(config);
    manager.tick(clock.millis());

    TEST_ASSERT_TRUE(manager.connecting());
    TEST_ASSERT_EQUAL(1, driver.beginStationCalls);
}

void test_connection_success_stops_setup_ap() {
    ManualClock clock;
    FakeWifiDriver driver;
    WifiManager manager(driver, clock);
    DeviceConfig config = defaultConfig();
    config.wifi.ssid = "office";

    manager.begin(config);
    manager.tick(clock.millis());
    driver.statusValue = WifiDriverStatus::Connected;
    manager.tick(clock.millis());

    TEST_ASSERT_TRUE(manager.connected());
    TEST_ASSERT_EQUAL(1, driver.stopApCalls);
}

void test_failed_connection_retries_and_falls_back() {
    ManualClock clock;
    FakeWifiDriver driver;
    WifiManager manager(driver, clock);
    DeviceConfig config = defaultConfig();
    config.wifi.ssid = "office";
    config.wifiRuntime.maxConnectRetries = 1;
    config.wifiRuntime.retryDelayMs = 10;

    manager.begin(config);
    manager.tick(clock.millis());
    driver.statusValue = WifiDriverStatus::Failed;
    manager.tick(clock.millis());
    TEST_ASSERT_EQUAL(1, manager.retryCount());

    clock.advance(20);
    driver.statusValue = WifiDriverStatus::Failed;
    manager.tick(clock.millis());
    TEST_ASSERT_TRUE(manager.provisioningFallback());
}

void test_connection_timeout_retry_resets_state_timer() {
    ManualClock clock;
    FakeWifiDriver driver;
    WifiManager manager(driver, clock);
    DeviceConfig config = defaultConfig();
    config.wifi.ssid = "office";
    config.wifiRuntime.connectTimeoutMs = 10;
    config.wifiRuntime.retryDelayMs = 1;
    config.wifiRuntime.maxConnectRetries = 3;

    manager.begin(config);
    manager.tick(clock.millis());
    TEST_ASSERT_EQUAL(1, driver.beginStationCalls);

    clock.advance(11);
    manager.tick(clock.millis());
    TEST_ASSERT_EQUAL(2, driver.beginStationCalls);
    TEST_ASSERT_EQUAL(1, manager.retryCount());

    clock.advance(1);
    manager.tick(clock.millis());
    TEST_ASSERT_EQUAL(2, driver.beginStationCalls);
    TEST_ASSERT_EQUAL(1, manager.retryCount());
}

void test_provisioning_coordinator_rejects_oversized_http_credentials() {
    ManualClock clock;
    FakeWifiDriver driver;
    MemoryConfigStorage storage;
    ConfigStore store(storage);
    TEST_ASSERT_TRUE(store.begin());
    TEST_ASSERT_TRUE(store.load().ok());

    WifiManager manager(driver, clock);
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

    WifiManager manager(driver, clock);
    manager.begin(store.config());
    ProvisioningCoordinator coordinator(store, manager);

    WiFiCredentials credentials;
    credentials.ssid = "office";
    credentials.password = "secret";
    TEST_ASSERT_EQUAL(static_cast<int>(ProvisioningResult::Accepted), static_cast<int>(coordinator.submitWifiCredentials(credentials)));

    coordinator.resetWifiCredentials();

    TEST_ASSERT_FALSE(store.config().wifi.hasCredentials());
    TEST_ASSERT_FALSE(manager.credentials().hasCredentials());
    TEST_ASSERT_TRUE(manager.provisioningFallback());
    TEST_ASSERT_EQUAL(1, driver.clearStationCredentialsCalls);
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

    WifiManager manager(driver, clock);
    manager.begin(store.config());
    ProvisioningCoordinator coordinator(store, manager);

    TEST_ASSERT_TRUE(coordinator.requestMobileProvisioningReentry());
    TEST_ASSERT_TRUE(coordinator.takeMobileProvisioningReentryRequest());
    TEST_ASSERT_FALSE(coordinator.takeMobileProvisioningReentryRequest());
}

void test_mobile_provisioning_timeout_uses_supplied_timestamp() {
    ManualClock clock;
    FakeWifiDriver driver;
    MemoryConfigStorage storage;
    ConfigStore store(storage);
    TEST_ASSERT_TRUE(store.begin());
    TEST_ASSERT_TRUE(store.load().ok());

    WifiManager manager(driver, clock);
    manager.begin(store.config());
    ProvisioningCoordinator coordinator(store, manager);
    MobileProvisioning provisioning(coordinator, clock);

    DeviceConfig config = store.config();
    config.provisioning.sessionTimeoutMs = 10;

    provisioning.begin(config);
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

    WifiManager manager(driver, clock);
    manager.begin(store.config());
    ProvisioningCoordinator coordinator(store, manager);
    MobileProvisioning provisioning(coordinator, clock);

    DeviceConfig config = store.config();
    config.provisioning.mobileProvisioningEnabled = true;
    config.provisioning.mobileBleTransport = false;

    provisioning.begin(config);
    provisioning.start(100);
    TEST_ASSERT_TRUE(provisioning.running());

    provisioning.restartBle(150);
    TEST_ASSERT_TRUE(provisioning.running());
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

    machine.loop(10);
    machine.requestRun();
    machine.loop(11);
    TEST_ASSERT_TRUE(machine.is((StateMachine::PState)&TestMachine::Running));

    machine.loop(21);
    TEST_ASSERT_TRUE(machine.is((StateMachine::PState)&TestMachine::TimedOut));
    TEST_ASSERT_TRUE(EWFM_SM_TIME_REACHED(5, 5));
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_no_credentials_enters_provisioning_fallback);
    RUN_TEST(test_credentials_start_station_connect);
    RUN_TEST(test_connection_success_stops_setup_ap);
    RUN_TEST(test_failed_connection_retries_and_falls_back);
    RUN_TEST(test_connection_timeout_retry_resets_state_timer);
    RUN_TEST(test_provisioning_coordinator_rejects_oversized_http_credentials);
    RUN_TEST(test_provisioning_coordinator_reset_clears_credentials_and_starts_softap);
    RUN_TEST(test_provisioning_coordinator_queues_mobile_reentry_request);
    RUN_TEST(test_mobile_provisioning_timeout_uses_supplied_timestamp);
    RUN_TEST(test_mobile_provisioning_restart_ble_keeps_session_active);
    RUN_TEST(test_portal_html_exposes_provisioning_reentry_action);
    RUN_TEST(test_state_machine_stack_and_return_to_popped_state);
    RUN_TEST(test_state_machine_pause_restart_and_updated_flag);
    RUN_TEST(test_state_machine_timeout_helpers);
    return UNITY_END();
}
