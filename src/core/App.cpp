#include "core/App.h"

#include "debug/Debug.h"

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
    : configStore_(storage_), wifiManager_(wifiDriver_, &configStore_), deviceRegistryStore_(deviceStorage_),
      retainedStateStore_(retainedStateStorage_),
      deviceRegistry_(deviceRegistryStore_, deviceTypeRegistry_, deviceIdSource_, &retainedStateStore_, &deviceEventDispatcher_),
      dashboardLayoutStore_(dashboardLayoutStorage_, &deviceRegistry_),
      portalServer_(wifiManager_, wifiDriver_, &deviceRegistry_, &deviceEventDispatcher_, &dashboardLayoutStore_) {}

bool App::begin() {
    EWFM_APP_LOG_INFO("ESP32 WiFi Manager booting");

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
        EWFM_BOOT_PRINTF("BOOT App::begin RetainedStateStore failed\n");
        return false;
    }
#if EWFM_BOOT_PROBE_STAGE <= 4
    EWFM_BOOT_PRINTF("BOOT App::begin probe stop after RetainedStateStore\n");
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
    EWFM_BOOT_PRINTF("BOOT App::begin done\n");

    return true;
}

void App::tick() {
    if (!begun_) {
        return;
    }
    const uint32_t now = clock_.millis();

    wifiManager_.tick(now);
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
    }

    deviceRegistry_.tick(now);
    if (due100ms) {
        deviceEventDispatcher_.tick100ms(now);
    }
    if (due1s) {
        deviceEventDispatcher_.tick1s(now);
    }
}

} // namespace ewfm
