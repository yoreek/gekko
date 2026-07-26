#include "config/ConfigStore.h"
#include "config/MemoryConfigStorage.h"
#include "time/NtpManager.h"
#include "wifi/WifiManager.h"

#include <unity.h>

using namespace ewfm;

namespace {

class FakeWifiDriver final : public IWifiDriver {
public:
    bool begin() override {
        networkStackReadyValue = beginResult;
        return beginResult;
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
        stationIpValue.clear();
    }
    bool startSetupAp(const std::string& ssid, const std::string& password) override {
        (void)ssid;
        (void)password;
        networkStackReadyValue = true;
        setupApActiveValue = startApResult;
        return startApResult;
    }
    void stopSetupAp() override {
        setupApActiveValue = false;
    }
    WifiDriverStatus status() const override {
        return statusValue;
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
    bool startApResult{true};
    bool beginResult{true};
    bool networkStackReadyValue{false};
    bool setupApActiveValue{false};
    std::string stationIpValue;
    std::string setupApIpValue{"192.168.4.1"};
};

class FakeNtpClient final : public INtpClient {
public:
    void beginResolve(const std::string& hostname) override {
        lastResolvedHostname = hostname;
        resolveStatus_ = resolveResult;
        ++beginResolveCalls;
    }
    NtpResolveStatus resolveStatus() const override {
        return resolveStatus_;
    }
    bool openSocket() override {
        ++openSocketCalls;
        return openSocketResult;
    }
    void closeSocket() override {
        ++closeSocketCalls;
    }
    bool sendRequest() override {
        ++sendRequestCalls;
        return sendRequestResult;
    }
    bool pollResponse(uint32_t& outEpochSeconds) override {
        if (!hasResponse) {
            return false;
        }
        outEpochSeconds = responseEpoch;
        return true;
    }

    std::string lastResolvedHostname;
    NtpResolveStatus resolveResult{NtpResolveStatus::Pending};
    NtpResolveStatus resolveStatus_{NtpResolveStatus::Idle};
    bool openSocketResult{true};
    bool sendRequestResult{true};
    bool hasResponse{false};
    uint32_t responseEpoch{0};
    int beginResolveCalls{0};
    int openSocketCalls{0};
    int closeSocketCalls{0};
    int sendRequestCalls{0};
};

} // namespace

void test_ntp_manager_waits_for_station_before_resolving() {
    FakeWifiDriver driver;
    WifiManager manager(driver);
    DeviceConfig config = defaultConfig();
    manager.begin(config);
    MemoryConfigStorage storage;
    ConfigStore configStore(storage);
    TEST_ASSERT_TRUE(configStore.begin());
    TEST_ASSERT_TRUE(configStore.load().ok());

    FakeNtpClient client;
    NtpManager ntp(client, configStore);
    ntp.begin(manager);

    ntp.tick(100);
    TEST_ASSERT_TRUE(ntp.waitingForStation());
    TEST_ASSERT_FALSE(ntp.synced());

    manager.tick(100);
    ntp.tick(100);
    manager.tick(101);
    ntp.tick(101);
    TEST_ASSERT_TRUE(ntp.waitingForStation());
    TEST_ASSERT_EQUAL(0, client.beginResolveCalls);
}

void test_ntp_manager_syncs_after_wifi_station_ready() {
    FakeWifiDriver driver;
    WifiManager manager(driver);
    DeviceConfig config = defaultConfig();
    config.wifi.ssid = "office";
    config.wifi.password = "secret";
    manager.begin(config);
    MemoryConfigStorage storage;
    ConfigStore configStore(storage);
    TEST_ASSERT_TRUE(configStore.begin());
    TEST_ASSERT_TRUE(configStore.load().ok());

    FakeNtpClient client;
    client.resolveResult = NtpResolveStatus::Resolved;
    client.hasResponse = true;
    client.responseEpoch = 1700000000UL;

    NtpManager ntp(client, configStore);
    ntp.begin(manager);

    ntp.tick(100);
    manager.tick(100);
    ntp.tick(100);
    manager.tick(101);
    ntp.tick(101);
    TEST_ASSERT_FALSE(ntp.synced());

    driver.statusValue = WifiDriverStatus::Connected;
    driver.stationIpValue = "192.168.1.240";
    manager.tick(102);
    ntp.tick(102); // WaitingForStation -> ResolveServer
    TEST_ASSERT_FALSE(ntp.synced());

    ntp.tick(103); // ResolveServer runs: beginResolve() sees Resolved immediately -> Syncing
    TEST_ASSERT_FALSE(ntp.synced());

    ntp.tick(104); // Syncing runs: opens socket + sends request -> CheckSynced
    TEST_ASSERT_FALSE(ntp.synced());

    ntp.tick(105); // CheckSynced runs: pollResponse succeeds -> Synced
    TEST_ASSERT_TRUE(ntp.synced());
    TEST_ASSERT_FALSE(ntp.waitingForStation());
    TEST_ASSERT_TRUE(ntp.lastSyncedUtc().has_value());
    TEST_ASSERT_EQUAL_UINT32(1700000000UL, ntp.lastSyncedUtc()->unixtime());
    TEST_ASSERT_EQUAL_STRING("pool.ntp.org", client.lastResolvedHostname.c_str());
    TEST_ASSERT_EQUAL(1, client.openSocketCalls);
    TEST_ASSERT_EQUAL(1, client.sendRequestCalls);
    TEST_ASSERT_EQUAL(1, client.closeSocketCalls);
}

void test_ntp_manager_resolve_timeout_retries_and_returns_to_waiting_for_station() {
    FakeWifiDriver driver;
    WifiManager manager(driver);
    DeviceConfig config = defaultConfig();
    config.wifi.ssid = "office";
    config.wifi.password = "secret";
    manager.begin(config);
    MemoryConfigStorage storage;
    ConfigStore configStore(storage);
    TEST_ASSERT_TRUE(configStore.begin());
    TEST_ASSERT_TRUE(configStore.load().ok());

    FakeNtpClient client;
    client.resolveResult = NtpResolveStatus::Pending; // never resolves

    NtpManager ntp(client, configStore);
    ntp.begin(manager);

    ntp.tick(100);
    manager.tick(100);
    ntp.tick(100);
    manager.tick(101);
    ntp.tick(101);

    driver.statusValue = WifiDriverStatus::Connected;
    driver.stationIpValue = "192.168.1.240";
    manager.tick(102);
    ntp.tick(102); // WaitingForStation -> ResolveServer (stateUpdated_ = 102)
    TEST_ASSERT_FALSE(ntp.waitingForStation());

    ntp.tick(30103); // beginResolve() called, still Pending; resolve timeout (30000ms) elapses -> RetryDelay
    TEST_ASSERT_FALSE(ntp.synced());
    TEST_ASSERT_FALSE(ntp.waitingForStation());

    ntp.tick(90104); // retry delay (60000ms) elapses -> WaitingForStation
    TEST_ASSERT_TRUE(ntp.waitingForStation());
    TEST_ASSERT_FALSE(ntp.synced());
}

void test_ntp_manager_response_timeout_retries() {
    FakeWifiDriver driver;
    WifiManager manager(driver);
    DeviceConfig config = defaultConfig();
    config.wifi.ssid = "office";
    config.wifi.password = "secret";
    manager.begin(config);
    MemoryConfigStorage storage;
    ConfigStore configStore(storage);
    TEST_ASSERT_TRUE(configStore.begin());
    TEST_ASSERT_TRUE(configStore.load().ok());

    FakeNtpClient client;
    client.resolveResult = NtpResolveStatus::Resolved;
    client.hasResponse = false; // never responds

    NtpManager ntp(client, configStore);
    ntp.begin(manager);

    ntp.tick(100);
    manager.tick(100);
    ntp.tick(100);
    manager.tick(101);
    ntp.tick(101);

    driver.statusValue = WifiDriverStatus::Connected;
    driver.stationIpValue = "192.168.1.240";
    manager.tick(102);
    ntp.tick(102); // WaitingForStation -> ResolveServer
    ntp.tick(103); // ResolveServer -> Syncing (resolved immediately)
    ntp.tick(104); // Syncing -> CheckSynced (stateUpdated_ = 104)
    TEST_ASSERT_FALSE(ntp.synced());

    ntp.tick(3105); // response timeout (3000ms) elapses -> RetryDelay
    TEST_ASSERT_FALSE(ntp.synced());
    TEST_ASSERT_FALSE(ntp.waitingForStation());
}

void test_ntp_manager_apply_settings_validates_and_updates_config() {
    FakeWifiDriver driver;
    WifiManager manager(driver);
    DeviceConfig config = defaultConfig();
    manager.begin(config);
    MemoryConfigStorage storage;
    ConfigStore configStore(storage);
    TEST_ASSERT_TRUE(configStore.begin());
    TEST_ASSERT_TRUE(configStore.load().ok());

    FakeNtpClient client;
    NtpManager ntp(client, configStore);
    ntp.begin(manager);

    TimeConfig invalid = ntp.config();
    invalid.timezoneId = "Not/AZone";
    const ValidationResult rejected = ntp.applySettings(invalid);
    TEST_ASSERT_FALSE(rejected.ok());
    TEST_ASSERT_EQUAL(static_cast<int>(ConfigError::TimezoneInvalid), static_cast<int>(rejected.error));
    TEST_ASSERT_EQUAL_STRING("Etc/GMT", ntp.config().timezoneId.c_str());

    TimeConfig valid = ntp.config();
    valid.ntpServer = "time.example.org";
    valid.timezoneId = "Europe/Kyiv";
    valid.syncIntervalSeconds = 7200;
    const ValidationResult accepted = ntp.applySettings(valid);
    TEST_ASSERT_TRUE(accepted.ok());
    TEST_ASSERT_EQUAL_STRING("time.example.org", ntp.config().ntpServer.c_str());
    TEST_ASSERT_EQUAL_STRING("Europe/Kyiv", ntp.config().timezoneId.c_str());
    TEST_ASSERT_EQUAL_UINT32(7200UL, ntp.config().syncIntervalSeconds);
}

void test_ntp_manager_disabled_never_syncs_even_when_station_ready() {
    FakeWifiDriver driver;
    WifiManager manager(driver);
    DeviceConfig config = defaultConfig();
    config.wifi.ssid = "office";
    config.wifi.password = "secret";
    manager.begin(config);
    MemoryConfigStorage storage;
    ConfigStore configStore(storage);
    TEST_ASSERT_TRUE(configStore.begin());
    TEST_ASSERT_TRUE(configStore.load().ok());

    FakeNtpClient client;
    client.resolveResult = NtpResolveStatus::Resolved;
    client.hasResponse = true;
    client.responseEpoch = 1700000000UL;

    NtpManager ntp(client, configStore);
    ntp.begin(manager);

    TimeConfig disabled = ntp.config();
    disabled.enabled = false;
    TEST_ASSERT_TRUE(ntp.applySettings(disabled).ok());
    TEST_ASSERT_FALSE(ntp.enabled());

    ntp.tick(100);
    manager.tick(100);
    ntp.tick(100);
    manager.tick(101);
    ntp.tick(101);

    driver.statusValue = WifiDriverStatus::Connected;
    driver.stationIpValue = "192.168.1.240";
    manager.tick(102);
    ntp.tick(102);

    // Station is ready, but NTP is disabled - the state machine must stay parked and never resolve.
    for (uint32_t t = 103; t < 200; ++t) {
        ntp.tick(t);
    }
    TEST_ASSERT_FALSE(ntp.synced());
    TEST_ASSERT_EQUAL(0, client.beginResolveCalls);

    TimeConfig enabled = ntp.config();
    enabled.enabled = true;
    TEST_ASSERT_TRUE(ntp.applySettings(enabled).ok());
    ntp.tick(200); // WaitingForStation -> ResolveServer now that it's enabled again
    ntp.tick(201); // ResolveServer -> Syncing
    ntp.tick(202); // Syncing -> CheckSynced
    ntp.tick(203); // CheckSynced -> Synced
    TEST_ASSERT_TRUE(ntp.synced());
}

void test_ntp_manager_manual_time_set_works_regardless_of_enabled() {
    FakeWifiDriver driver;
    WifiManager manager(driver);
    DeviceConfig config = defaultConfig();
    manager.begin(config);
    MemoryConfigStorage storage;
    ConfigStore configStore(storage);
    TEST_ASSERT_TRUE(configStore.begin());
    TEST_ASSERT_TRUE(configStore.load().ok());

    FakeNtpClient client;
    NtpManager ntp(client, configStore);
    ntp.begin(manager);

    TimeConfig disabled = ntp.config();
    disabled.enabled = false;
    TEST_ASSERT_TRUE(ntp.applySettings(disabled).ok());

    TEST_ASSERT_FALSE(ntp.synced());
    ntp.setManualTime(1700000000UL);
    TEST_ASSERT_TRUE(ntp.synced());
    TEST_ASSERT_TRUE(ntp.lastSyncedUtc().has_value());
    TEST_ASSERT_EQUAL_UINT32(1700000000UL, ntp.lastSyncedUtc()->unixtime());
    TEST_ASSERT_TRUE(TimeSource::Manual == ntp.lastSource());
}

void test_ntp_manager_manual_time_counts_as_authoritative_sync_and_bumps_revision() {
    FakeWifiDriver driver;
    WifiManager manager(driver);
    DeviceConfig config = defaultConfig();
    manager.begin(config);
    MemoryConfigStorage storage;
    ConfigStore configStore(storage);
    TEST_ASSERT_TRUE(configStore.begin());
    TEST_ASSERT_TRUE(configStore.load().ok());

    FakeNtpClient client;
    NtpManager ntp(client, configStore);
    ntp.begin(manager);
    ntp.tick(50);

    TEST_ASSERT_FALSE(ntp.hasAuthoritativeSync());
    TEST_ASSERT_EQUAL_UINT32(0, ntp.syncRevision());

    ntp.setManualTime(1700000000UL);
    TEST_ASSERT_TRUE(ntp.hasAuthoritativeSync());
    TEST_ASSERT_EQUAL_UINT32(1, ntp.syncRevision());
    TEST_ASSERT_EQUAL_UINT32(50, ntp.lastAuthoritativeSyncMs());

    ntp.setManualTime(1700000100UL);
    TEST_ASSERT_EQUAL_UINT32(2, ntp.syncRevision());
}

void test_ntp_manager_seed_from_rtc_does_not_count_as_authoritative_sync() {
    FakeWifiDriver driver;
    WifiManager manager(driver);
    DeviceConfig config = defaultConfig();
    manager.begin(config);
    MemoryConfigStorage storage;
    ConfigStore configStore(storage);
    TEST_ASSERT_TRUE(configStore.begin());
    TEST_ASSERT_TRUE(configStore.load().ok());

    FakeNtpClient client;
    NtpManager ntp(client, configStore);
    ntp.begin(manager);
    ntp.tick(50);

    ntp.seedFromRtc(1700000000UL);
    TEST_ASSERT_TRUE(ntp.synced());
    TEST_ASSERT_TRUE(TimeSource::Rtc == ntp.lastSource());
    TEST_ASSERT_TRUE(ntp.lastSyncedUtc().has_value());
    TEST_ASSERT_EQUAL_UINT32(1700000000UL, ntp.lastSyncedUtc()->unixtime());

    // RTC seeds are a stand-in, not an authoritative sync - NTP must keep retrying independently
    // and RtcSyncCoordinator must not perceive this as a fresh sync worth writing back.
    TEST_ASSERT_FALSE(ntp.hasAuthoritativeSync());
    TEST_ASSERT_EQUAL_UINT32(0, ntp.syncRevision());

    ntp.setManualTime(1700000500UL);
    TEST_ASSERT_TRUE(ntp.hasAuthoritativeSync());
    TEST_ASSERT_EQUAL_UINT32(1, ntp.syncRevision());
    TEST_ASSERT_TRUE(TimeSource::Manual == ntp.lastSource());
}
