#include "core/App.h"

#include "debug/Debug.h"

#if defined(ARDUINO) && !defined(UNIT_TEST)
#include <LittleFS.h>
#endif

namespace ewfm {

namespace {
constexpr uint32_t kTick100msIntervalMs = 100;
constexpr uint32_t kTick1sIntervalMs = 1000;
} // namespace

App::App()
    : configStore_(storage_), wifiManager_(wifiDriver_, &configStore_), deviceRegistryStore_(deviceStorage_),
      retainedStateStore_(retainedStateStorage_),
      deviceRegistry_(deviceRegistryStore_, deviceTypeRegistry_, deviceIdSource_, &retainedStateStore_, &deviceEventDispatcher_),
      dashboardLayoutStore_(dashboardLayoutStorage_, &deviceRegistry_),
      portalServer_(wifiManager_, wifiDriver_, &deviceRegistry_, &deviceEventDispatcher_, &dashboardLayoutStore_) {}

bool App::begin() {
    EWFM_APP_LOG_INFO("ESP32 WiFi Manager booting");

#if defined(ARDUINO) && !defined(UNIT_TEST)
    const bool littleFsMounted = LittleFS.begin(true, "/littlefs", 10, "littlefs");
    EWFM_APP_LOG_INFO("LittleFS mount result=%d total=%lu used=%lu index=%d favicon=%d", static_cast<int>(littleFsMounted),
                      static_cast<unsigned long>(LittleFS.totalBytes()), static_cast<unsigned long>(LittleFS.usedBytes()),
                      static_cast<int>(LittleFS.exists("/index.html.gz")), static_cast<int>(LittleFS.exists("/favicon.svg.gz")));
#endif

    if (!configStore_.begin()) {
        EWFM_APP_LOG_INFO("ConfigStore begin failed");
        return false;
    }
    if (!deviceRegistryStore_.begin(false)) {
        EWFM_APP_LOG_INFO("DeviceRegistryStore begin failed");
        return false;
    }
    if (!retainedStateStore_.begin(false)) {
        EWFM_APP_LOG_INFO("RetainedStateStore begin failed");
        return false;
    }
    if (!dashboardLayoutStore_.begin()) {
        EWFM_APP_LOG_INFO("DashboardLayoutStore begin failed");
        return false;
    }

    ValidationResult loaded = configStore_.load();
    if (!loaded.ok()) {
        EWFM_APP_LOG_INFO("Config load warning: %s", loaded.message);
    }

    const DeviceConfig& config = configStore_.config();
    const uint32_t now = clock_.millis();
    const DeviceValidationResult registryResult = deviceRegistry_.begin(now);
    if (!registryResult.ok()) {
        EWFM_APP_LOG_INFO("Device registry load failed: %s", registryResult.message);
    }
    lastTick100ms_ = now;
    lastTick1s_ = now;
    wifiManager_.begin(config);
    begun_ = true;
    if (!portalServer_.begin()) {
        EWFM_APP_LOG_INFO("PortalServer begin failed");
    }
#if defined(WITH_ARDUINO_OTA)
    otaService_.begin(config.deviceName, wifiManager_);
#endif

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
