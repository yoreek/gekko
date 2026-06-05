#include "config/ConfigStore.h"
#include "config/MemoryConfigStorage.h"
#include "core/Clock.h"
#include "core/StateMachine.h"
#include "provisioning/MobileProvisioning.h"
#include "provisioning/ProvisioningCoordinator.h"
#include "wifi/WifiManager.h"

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
    int startApCalls{0};
    int stopApCalls{0};
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
    RUN_TEST(test_mobile_provisioning_timeout_uses_supplied_timestamp);
    RUN_TEST(test_state_machine_stack_and_return_to_popped_state);
    RUN_TEST(test_state_machine_pause_restart_and_updated_flag);
    RUN_TEST(test_state_machine_timeout_helpers);
    return UNITY_END();
}
