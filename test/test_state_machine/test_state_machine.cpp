#include "config/ConfigStore.h"
#include "config/MemoryConfigStorage.h"
#include "core/Clock.h"
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

void test_no_credentials_enters_provisioning_fallback() {
    ManualClock clock;
    FakeWifiDriver driver;
    WifiManager manager(driver, clock);
    DeviceConfig config = defaultConfig();

    manager.begin(config);

    TEST_ASSERT_EQUAL(static_cast<int>(WifiManagerState::ProvisioningFallback), static_cast<int>(manager.state()));
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

    TEST_ASSERT_EQUAL(static_cast<int>(WifiManagerState::Connecting), static_cast<int>(manager.state()));
    TEST_ASSERT_EQUAL(1, driver.beginStationCalls);
}

void test_connection_success_stops_setup_ap() {
    ManualClock clock;
    FakeWifiDriver driver;
    WifiManager manager(driver, clock);
    DeviceConfig config = defaultConfig();
    config.wifi.ssid = "office";

    manager.begin(config);
    driver.statusValue = WifiDriverStatus::Connected;
    manager.tick();

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
    driver.statusValue = WifiDriverStatus::Failed;
    manager.tick();
    TEST_ASSERT_EQUAL(1, manager.retryCount());

    clock.advance(20);
    driver.statusValue = WifiDriverStatus::Failed;
    manager.tick();
    TEST_ASSERT_EQUAL(static_cast<int>(WifiManagerState::ProvisioningFallback), static_cast<int>(manager.state()));
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
    TEST_ASSERT_EQUAL(1, driver.beginStationCalls);

    clock.advance(11);
    manager.tick();
    TEST_ASSERT_EQUAL(2, driver.beginStationCalls);
    TEST_ASSERT_EQUAL(1, manager.retryCount());

    clock.advance(1);
    manager.tick();
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

void test_state_machine_stack_and_return_to_popped_state() {
    StateMachine<WifiManagerState> sm(WifiManagerState::Idle);
    TEST_ASSERT_TRUE(sm.push(WifiManagerState::Connected));
    sm.transitionTo(WifiManagerState::Connecting, 10);

    TEST_ASSERT_TRUE(sm.returnToPopped(20));
    TEST_ASSERT_EQUAL(static_cast<int>(WifiManagerState::Connected), static_cast<int>(sm.state()));
    TEST_ASSERT_EQUAL_UINT32(20, sm.enteredAt());
}

void test_state_machine_pause_restart_and_updated_flag() {
    StateMachine<WifiManagerState> sm(WifiManagerState::Idle);
    sm.transitionTo(WifiManagerState::Connecting, 10);
    TEST_ASSERT_TRUE(sm.isUpdated());

    sm.transitionTo(WifiManagerState::Connecting, 20);
    TEST_ASSERT_FALSE(sm.isUpdated());

    sm.pause();
    TEST_ASSERT_TRUE(sm.isPaused());
    sm.restart();
    TEST_ASSERT_FALSE(sm.isPaused());
}

void test_state_machine_timeout_helpers() {
    StateMachine<WifiManagerState> sm(WifiManagerState::Idle);

    sm.transitionTo(WifiManagerState::Connecting, 10);
    TEST_ASSERT_FALSE(EWFM_SM_TIMEOUT(sm, 19, 10));
    TEST_ASSERT_TRUE(EWFM_SM_TIMEOUT(sm, 20, 10));

    sm.resetTimer(20);
    TEST_ASSERT_FALSE(EWFM_SM_TIMEOUT(sm, 29, 10));
    TEST_ASSERT_TRUE(EWFM_SM_TIMEOUT(sm, 30, 10));
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
    RUN_TEST(test_state_machine_stack_and_return_to_popped_state);
    RUN_TEST(test_state_machine_pause_restart_and_updated_flag);
    RUN_TEST(test_state_machine_timeout_helpers);
    return UNITY_END();
}
