#include "config/ConfigStore.h"
#include "config/MemoryConfigStorage.h"
#include "devices/core/DeviceIdGenerator.h"
#include "devices/core/DeviceRuntimeBase.h"
#include "devices/registry/DeviceRegistry.h"
#include "time/NtpClient.h"
#include "time/NtpManager.h"
#include "time/RtcSyncCoordinator.h"

#include <unity.h>

using namespace ewfm;

namespace {

// RtcSyncCoordinator only ever talks to a DeviceRegistry entry through IRealTimeClockRuntime, so
// this fake stands in for Ds3231RtcDevice without needing real I2C hardware (which never
// initializes under env:native - see I2cBusDevice/ArduinoI2cBusDriver's NullI2cBusDriver
// fallback). It is Ready from construction, no dependency wiring or tick-driven lifecycle needed.
class FakeRtcRuntime final : public DeviceRuntimeBase, public IRealTimeClockRuntime {
public:
    FakeRtcRuntime() : DeviceRuntimeBase((PState)&FakeRtcRuntime::Idle) {
        status_ = DeviceStatus::Ready;
    }

    bool handleCommand(const DeviceCommand&) override {
        return false;
    }
    bool applyConfig(const DeviceConfigBlob&, uint32_t) override {
        return true;
    }
    bool serializeConfigBlob(DeviceConfigBlob&) const override {
        return true;
    }
    const IRealTimeClockRuntime* realTimeClockRuntime() const override {
        return this;
    }
    bool latestTimeReading(uint32_t& outUtcEpoch, bool& outLostPower) const override {
        if (!hasReading) {
            return false;
        }
        outUtcEpoch = epoch;
        outLostPower = lostPower;
        return true;
    }
    bool writeTime(uint32_t utcEpoch) override {
        ++writeCount;
        lastWrittenEpoch = utcEpoch;
        return true;
    }
    bool useForSystemTimeSync() const override {
        return activeSync;
    }

    static DeviceTypeDescriptor descriptor() {
        DeviceTypeDescriptor descriptor;
        descriptor.typeId = kTypeId;
        descriptor.name = "FakeRtcRuntime";
        descriptor.currentConfigVersion = 1;
        descriptor.providedRoles = ProvidedRoles::of({DeviceRole::RealTimeClock});
        descriptor.createRuntime = &FakeRtcRuntime::createRuntime;
        return descriptor;
    }
    static std::unique_ptr<IDeviceRuntime> createRuntime(const DeviceRegistryEntry&, const DeviceConfigBlob&) {
        return std::unique_ptr<IDeviceRuntime>(new FakeRtcRuntime());
    }

    static constexpr DeviceTypeId kTypeId = 9000;

    bool hasReading{true};
    uint32_t epoch{0};
    bool lostPower{false};
    bool activeSync{true};
    uint32_t writeCount{0};
    uint32_t lastWrittenEpoch{0};

private:
    State Idle() {}
};

class NeverRespondingNtpClient final : public INtpClient {
public:
    void beginResolve(const std::string&) override {}
    NtpResolveStatus resolveStatus() const override {
        return NtpResolveStatus::Pending;
    }
    bool openSocket() override {
        return true;
    }
    void closeSocket() override {}
    bool sendRequest() override {
        return true;
    }
    bool pollResponse(uint32_t&) override {
        return false;
    }
};

DeviceTypeRegistry makeTypeRegistryWithFakeRtc() {
    DeviceTypeRegistry types;
    TEST_ASSERT_TRUE(types.registerDescriptor(FakeRtcRuntime::descriptor()));
    return types;
}

struct Harness {
    MemoryConfigStorage registryStorage;
    DeviceRegistryStore store{registryStorage};
    SequentialDeviceIdSource ids{8000};
    DeviceTypeRegistry types{makeTypeRegistryWithFakeRtc()};
    DeviceRegistry registry{store, types, ids};
    MemoryConfigStorage timeConfigStorage;
    ConfigStore configStore{timeConfigStorage};
    NeverRespondingNtpClient ntpClient;
    NtpManager ntpManager{ntpClient, configStore};
    RtcSyncCoordinator coordinator{ntpManager, registry};
    FakeRtcRuntime* rtc{nullptr};

    void begin() {
        TEST_ASSERT_TRUE(configStore.begin());
        TEST_ASSERT_TRUE(configStore.load().ok());
        TEST_ASSERT_TRUE(store.begin(false));
        TEST_ASSERT_TRUE(registry.begin(0).ok());

        DeviceCreateRequest request{};
        request.typeId = FakeRtcRuntime::kTypeId;
        TEST_ASSERT_TRUE(request.assignName("rtc"));
        request.setEnabled(true);
        const DeviceCreateResult result = registry.create(request, 1);
        TEST_ASSERT_TRUE_MESSAGE(result.ok(), result.validation.message);
        rtc = static_cast<FakeRtcRuntime*>(registry.runtime(result.deviceId));
        TEST_ASSERT_NOT_NULL(rtc);
    }
};

} // namespace

void test_rtc_sync_coordinator_seeds_system_time_from_rtc_before_first_ntp_sync() {
    Harness harness;
    harness.begin();
    harness.rtc->epoch = 1700000000UL;

    TEST_ASSERT_FALSE(harness.ntpManager.hasAuthoritativeSync());
    harness.coordinator.tick(31000); // coordinator gates its own tick to kTickIntervalMs (30s)

    TEST_ASSERT_TRUE(harness.ntpManager.synced());
    TEST_ASSERT_TRUE(TimeSource::Rtc == harness.ntpManager.lastSource());
    TEST_ASSERT_TRUE(harness.ntpManager.lastSyncedUtc().has_value());
    TEST_ASSERT_EQUAL_UINT32(1700000000UL, harness.ntpManager.lastSyncedUtc()->unixtime());
    // A boot-time RTC seed is a stand-in, not a real sync: NTP must keep retrying on its own.
    TEST_ASSERT_FALSE(harness.ntpManager.hasAuthoritativeSync());
}

void test_rtc_sync_coordinator_ignores_rtc_device_not_opted_in() {
    Harness harness;
    harness.begin();
    harness.rtc->epoch = 1700000000UL;
    harness.rtc->activeSync = false;

    harness.coordinator.tick(31000); // coordinator gates its own tick to kTickIntervalMs (30s)
    TEST_ASSERT_FALSE(harness.ntpManager.synced());
}

void test_rtc_sync_coordinator_writes_back_after_authoritative_sync() {
    Harness harness;
    harness.begin();
    harness.rtc->epoch = 1700000000UL;

    // Each tick() call below must be spaced out by more than kTickIntervalMs (30s) - the
    // coordinator gates its own tick and otherwise silently no-ops on the intermediate calls.
    harness.coordinator.tick(31000); // boot seed, not authoritative, no write-back yet
    TEST_ASSERT_EQUAL_UINT32(0, harness.rtc->writeCount);

    harness.ntpManager.setManualTime(1700005000UL); // authoritative
    harness.coordinator.tick(62000);
    TEST_ASSERT_EQUAL_UINT32(1, harness.rtc->writeCount);
    TEST_ASSERT_EQUAL_UINT32(1700005000UL, harness.rtc->lastWrittenEpoch);

    // No new authoritative sync and well under the periodic write-back interval - no repeat write.
    harness.coordinator.tick(93000);
    TEST_ASSERT_EQUAL_UINT32(1, harness.rtc->writeCount);
}

void test_rtc_sync_coordinator_falls_back_to_rtc_after_prolonged_ntp_outage() {
    Harness harness;
    harness.begin();
    harness.rtc->epoch = 1700000000UL;

    harness.ntpManager.setManualTime(1700000000UL);
    harness.coordinator.tick(31000); // coordinator gates its own tick to kTickIntervalMs (30s)
    TEST_ASSERT_TRUE(harness.ntpManager.hasAuthoritativeSync());
    const uint32_t authoritativeSyncMs = harness.ntpManager.lastAuthoritativeSyncMs();

    // Simulate a long stretch with no fresh NTP sync (> 6h staleness threshold) and an RTC that
    // has since drifted to a different (fresher, in a real chip) reading.
    harness.rtc->epoch = 1700030000UL;
    const uint32_t staleNow = authoritativeSyncMs + 7UL * 60UL * 60UL * 1000UL;
    harness.coordinator.tick(staleNow);

    TEST_ASSERT_TRUE(TimeSource::Rtc == harness.ntpManager.lastSource());
    TEST_ASSERT_TRUE(harness.ntpManager.lastSyncedUtc().has_value());
    TEST_ASSERT_EQUAL_UINT32(1700030000UL, harness.ntpManager.lastSyncedUtc()->unixtime());
    // The staleness fallback must not fabricate a fresh authoritative sync - NTP should still be
    // considered overdue so it keeps retrying independently.
    TEST_ASSERT_TRUE(harness.ntpManager.hasAuthoritativeSync());
    TEST_ASSERT_EQUAL_UINT32(authoritativeSyncMs, harness.ntpManager.lastAuthoritativeSyncMs());
}
