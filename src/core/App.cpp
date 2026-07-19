#include "core/App.h"

#include "debug/Debug.h"
#include "generated/Version.h"

#if defined(ARDUINO) && !defined(UNIT_TEST)
#include <LittleFS.h>
#include <esp_rom_sys.h>
#endif

namespace ewfm {

namespace {
constexpr uint32_t kTick100msIntervalMs = 100;
constexpr uint32_t kTick1sIntervalMs = 1000;

// Temporary boot probe stage. Increase this one step at a time while tracing
// the first failing call in App::begin().
#ifndef EWFM_BOOT_PROBE_STAGE
#define EWFM_BOOT_PROBE_STAGE 100
#endif

#if defined(ARDUINO) && !defined(UNIT_TEST) && defined(ESP32)
#define EWFM_BOOT_PRINTF(...) esp_rom_printf(__VA_ARGS__)
#else
#define EWFM_BOOT_PRINTF(...)                                                                                                              \
    do {                                                                                                                                   \
    } while (false)
#endif
} // namespace

App::App()
    : configStore_(storage_), wifiManager_(wifiDriver_, &configStore_), ntpManager_(ntpClient_, configStore_),
      deviceRegistryStore_(deviceStorage_), retainedStateStore_(retainedStateStorage_), deviceScopedDataStore_(displayLayoutStorage_),
      displayLayoutStore_(displayLayoutStorage_), deviceRegistry_(deviceRegistryStore_, deviceTypeRegistry_, deviceIdSource_,
                                                                  &retainedStateStore_, &deviceScopedDataStore_, &deviceEventDispatcher_),
      rtcSyncCoordinator_(ntpManager_, deviceRegistry_), displayRenderCoordinator_(deviceRegistry_, wifiDriver_),
      dashboardLayoutStore_(dashboardLayoutStorage_, &deviceRegistry_),
      portalServer_(wifiManager_, wifiDriver_, &deviceRegistry_, &deviceEventDispatcher_, &dashboardLayoutStore_,
#if defined(WITH_HOME_ASSISTANT)
                    &mqttConfigStore_, &mqttManager_, &deviceScopedDataStore_, &haDiscoveryBridge_,
#else
                    nullptr, nullptr, nullptr, nullptr,
#endif
                    &ntpManager_) {
}

bool App::begin() {
    EWFM_APP_LOG_INFO("Gekko booting version=%s build=%s", EWFM_FIRMWARE_VERSION, EWFM_FIRMWARE_BUILD_DATE);

#if defined(ARDUINO) && !defined(UNIT_TEST) && defined(ESP32)
    EWFM_BOOT_PRINTF("BOOT App::begin LittleFS start\n");
    const bool littleFsMounted = LittleFS.begin(true, "/littlefs", 10, "littlefs");
    EWFM_BOOT_PRINTF("BOOT App::begin LittleFS mounted=%d total=%lu used=%lu index=%d favicon=%d\n", static_cast<int>(littleFsMounted),
                     static_cast<unsigned long>(LittleFS.totalBytes()), static_cast<unsigned long>(LittleFS.usedBytes()),
                     static_cast<int>(LittleFS.exists("/index.html.gz")), static_cast<int>(LittleFS.exists("/favicon.svg.gz")));
#if EWFM_BOOT_PROBE_STAGE <= 1
    EWFM_BOOT_PRINTF("BOOT App::begin probe stop after LittleFS\n");
    return littleFsMounted;
#endif
#endif

    // Dose journal mounts its own devdata partition (formatted on first boot); a missing/failed
    // journal must never block boot - dosing devices tolerate a null journal by dropping records.
    (void)doseJournalStorage_.begin();
    setDefaultDoseJournal(&doseJournal_);
    (void)deviceEventDispatcher_.registerSink(doseJournalCleanupSink_);

    EWFM_BOOT_PRINTF("BOOT App::begin configStore\n");
    if (!configStore_.begin()) {
        EWFM_BOOT_PRINTF("BOOT App::begin ConfigStore failed\n");
        return false;
    }
#if EWFM_BOOT_PROBE_STAGE <= 2
    EWFM_BOOT_PRINTF("BOOT App::begin probe stop after ConfigStore\n");
    return true;
#endif
    EWFM_BOOT_PRINTF("BOOT App::begin deviceRegistryStore\n");
    if (!deviceRegistryStore_.begin(false)) {
        EWFM_BOOT_PRINTF("BOOT App::begin DeviceRegistryStore failed\n");
        return false;
    }
#if EWFM_BOOT_PROBE_STAGE <= 3
    EWFM_BOOT_PRINTF("BOOT App::begin probe stop after DeviceRegistryStore\n");
    return true;
#endif
    EWFM_BOOT_PRINTF("BOOT App::begin retainedStateStore\n");
    if (!retainedStateStore_.begin(false)) {
        EWFM_BOOT_PRINTF("BOOT App::begin DeviceRetainedDataStore failed\n");
        return false;
    }
#if EWFM_BOOT_PROBE_STAGE <= 4
    EWFM_BOOT_PRINTF("BOOT App::begin probe stop after DeviceRetainedDataStore\n");
    return true;
#endif
    EWFM_BOOT_PRINTF("BOOT App::begin displayLayoutStore\n");
    if (!displayLayoutStore_.begin(false)) {
        EWFM_BOOT_PRINTF("BOOT App::begin DisplayLayoutStore failed\n");
        return false;
    }
#if EWFM_BOOT_PROBE_STAGE <= 4
    EWFM_BOOT_PRINTF("BOOT App::begin probe stop after DisplayLayoutStore\n");
    return true;
#endif
    EWFM_BOOT_PRINTF("BOOT App::begin dashboardLayoutStore\n");
    if (!dashboardLayoutStore_.begin()) {
        EWFM_BOOT_PRINTF("BOOT App::begin DashboardLayoutStore failed\n");
        return false;
    }
#if EWFM_BOOT_PROBE_STAGE <= 5
    EWFM_BOOT_PRINTF("BOOT App::begin probe stop after DashboardLayoutStore\n");
    return true;
#endif

#if defined(WITH_HOME_ASSISTANT)
    EWFM_BOOT_PRINTF("BOOT App::begin mqttConfigStore\n");
    if (!mqttConfigStore_.begin()) {
        EWFM_BOOT_PRINTF("BOOT App::begin MqttConfigStore failed\n");
        return false;
    }
    mqttConfigStore_.load();
#endif

    EWFM_BOOT_PRINTF("BOOT App::begin config load\n");
    ValidationResult loaded = configStore_.load();
    if (!loaded.ok()) {
        EWFM_BOOT_PRINTF("BOOT App::begin Config load warning: %s\n", loaded.message);
    }
#if EWFM_BOOT_PROBE_STAGE <= 6
    EWFM_BOOT_PRINTF("BOOT App::begin probe stop after ConfigLoad\n");
    return true;
#endif

    const DeviceConfig& config = configStore_.config();
#if defined(WITH_HOME_ASSISTANT)
    if (mqttConfigStore_.settings().haNodeId.empty()) {
        MqttSettings seeded = mqttConfigStore_.settings();
        seeded.haNodeId = defaultHaNodeId(config.deviceName, wifiDriver_.macSuffix());
        mqttConfigStore_.save(seeded);
    }
#endif
    const uint32_t now = clock_.millis();
    EWFM_BOOT_PRINTF("BOOT App::begin deviceRegistry begin\n");
    const DeviceValidationResult registryResult = deviceRegistry_.begin(now);
    if (!registryResult.ok()) {
        EWFM_BOOT_PRINTF("BOOT App::begin Device registry load failed: %s\n", registryResult.message);
    }
#if EWFM_BOOT_PROBE_STAGE <= 7
    EWFM_BOOT_PRINTF("BOOT App::begin probe stop after DeviceRegistryBegin\n");
    return true;
#endif
    lastTick100ms_ = now;
    lastTick1s_ = now;
    EWFM_BOOT_PRINTF("BOOT App::begin wifiManager begin\n");
    wifiManager_.begin(config);
#if EWFM_BOOT_PROBE_STAGE <= 8
    EWFM_BOOT_PRINTF("BOOT App::begin probe stop after WifiManagerBegin\n");
    return true;
#endif
    EWFM_BOOT_PRINTF("BOOT App::begin ntpManager begin\n");
    ntpManager_.begin(wifiManager_);
    begun_ = true;
    EWFM_BOOT_PRINTF("BOOT App::begin portal begin\n");
    if (!portalServer_.begin()) {
        EWFM_BOOT_PRINTF("BOOT App::begin PortalServer begin failed\n");
    }
#if EWFM_BOOT_PROBE_STAGE <= 11
    EWFM_BOOT_PRINTF("BOOT App::begin probe stop after PortalServerBegin\n");
    return true;
#endif
#if defined(WITH_ARDUINO_OTA)
    EWFM_BOOT_PRINTF("BOOT App::begin ota begin\n");
    otaService_.begin(config.deviceName, wifiManager_);
#endif
#if defined(WITH_HOME_ASSISTANT)
    EWFM_BOOT_PRINTF("BOOT App::begin mqttManager begin\n");
    mqttManager_.begin(wifiManager_);
    {
        std::vector<uint8_t> caCert;
        mqttConfigStore_.loadCaCert(caCert);
        mqttManager_.applySettings(mqttConfigStore_.settings(), caCert);
    }
    haDiscoveryBridge_.begin(mqttConfigStore_.settings().haNodeId, mqttConfigStore_.settings().haNodeName,
                             mqttConfigStore_.settings().haDiscoveryPrefix);
    haDiscoveryBridge_.attachDispatcher();
    systemHaPublisher_.begin(mqttConfigStore_.settings().haNodeId, mqttConfigStore_.settings().haNodeName,
                             mqttConfigStore_.settings().haDiscoveryPrefix);
#endif
    EWFM_BOOT_PRINTF("BOOT App::begin done\n");

    return true;
}

void App::tick() {
    if (!begun_) {
        return;
    }
    const uint32_t now = clock_.millis();

    wifiManager_.tick(now);
    ntpManager_.tick(now);
#if defined(WITH_HOME_ASSISTANT)
    mqttManager_.tick(now);
    systemHaPublisher_.tick(now);
#endif
    portalServer_.tick(now);
    tickDeviceCadence(now);
    deviceEventDispatcher_.tickFastLoop(now);
#if defined(WITH_ARDUINO_OTA)
    otaService_.tick(now);
#endif
}

void App::tickDeviceCadence(uint32_t now) {
    const bool due100ms = static_cast<uint32_t>(now - lastTick100ms_) >= kTick100msIntervalMs;
    const bool due1s = static_cast<uint32_t>(now - lastTick1s_) >= kTick1sIntervalMs;

    deviceRegistry_.tickFastLoop(now);

    if (due100ms) {
        lastTick100ms_ = now;
        deviceRegistry_.tick100ms(now);
    }

    if (due1s) {
        lastTick1s_ = now;
        deviceRegistry_.tick1s(now);
        // RtcSyncCoordinator's own policy only ever acts on minutes-to-hours timescales, so
        // there's no value driving it off the fast loop - reuse this cadence gate instead of a
        // dedicated one.
        rtcSyncCoordinator_.tick(now);
    }

    deviceRegistry_.tick(now);
    displayRenderCoordinator_.tick(now);
    if (due100ms) {
        deviceEventDispatcher_.tick100ms(now);
    }
    if (due1s) {
        deviceEventDispatcher_.tick1s(now);
    }
}

} // namespace ewfm
