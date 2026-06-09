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
      deviceRegistry_(deviceRegistryStore_, deviceTypeRegistry_, deviceIdSource_, nullptr, &deviceEventDispatcher_),
      portalServer_(wifiManager_, wifiDriver_, &deviceRegistry_) {}

bool App::begin() {
    EWFM_APP_LOG_INFO("ESP32 WiFi Manager booting");

#if defined(ARDUINO) && !defined(UNIT_TEST)
    LittleFS.begin(true, "/littlefs", 10, "littlefs");
#endif

    if (!configStore_.begin()) {
        EWFM_APP_LOG_INFO("ConfigStore begin failed");
        return false;
    }
    if (!deviceRegistryStore_.begin(false)) {
        EWFM_APP_LOG_INFO("DeviceRegistryStore begin failed");
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
        return false;
    }
    lastTick100ms_ = now;
    lastTick1s_ = now;
    wifiManager_.begin(config);
    portalServer_.begin();
#if defined(WITH_ARDUINO_OTA)
    otaService_.begin(config.deviceName, wifiManager_);
#endif

    return true;
}

void App::tick() {
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
