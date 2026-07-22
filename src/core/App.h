#pragma once

#include "config/ConfigStore.h"
#if defined(WITH_HOME_ASSISTANT)
#include "config/MqttConfigStore.h"
#include "integrations/mqtt/HaDiscoveryBridge.h"
#include "integrations/mqtt/system/SystemHaPublisher.h"
#include "platform/ArduinoSystemStats.h"
#include "platform/MqttManager.h"
#endif
#include "devices/analog/scheduled/presets/LittleFsSchedulePresetStorage.h"
#include "devices/analog/scheduled/presets/SchedulePresetCleanupSink.h"
#include "devices/core/DeviceIdGenerator.h"
#include "devices/core/DeviceTypes.h"
#include "devices/display/DisplayLayoutStore.h"
#include "devices/display/DisplayRenderCoordinator.h"
#include "devices/dosing/journal/DoseJournalCleanupSink.h"
#include "devices/dosing/journal/LittleFsDoseJournalStorage.h"
#include "devices/dosing/journal/SegmentedDoseJournal.h"
#include "devices/registry/DeviceRegistry.h"
#include "devices/registry/DeviceRegistryStore.h"
#include "devices/registry/DeviceRetainedDataStore.h"
#include "devices/registry/DeviceScopedDataStore.h"
#include "integrations/common/DeviceEventDispatcher.h"
#include "platform/ArduinoClock.h"
#include "platform/ArduinoNtpClient.h"
#include "platform/ArduinoOtaService.h"
#include "platform/ArduinoWifiDriver.h"
#include "platform/PreferencesConfigStorage.h"
#include "portal/DashboardLayoutStore.h"
#include "portal/PortalServer.h"
#include "time/NtpManager.h"
#include "time/RtcSyncCoordinator.h"
#include "wifi/WifiManager.h"

#include <memory>

namespace ewfm {

class App {
public:
    App();
    bool begin();
    void tick();

private:
    void tickDeviceCadence(uint32_t now);

    ArduinoClock clock_;
    PreferencesConfigStorage storage_;
    PreferencesConfigStorage deviceStorage_;
    PreferencesConfigStorage retainedStateStorage_;
    PreferencesConfigStorage displayLayoutStorage_;
    PreferencesConfigStorage dashboardLayoutStorage_;
    ArduinoWifiDriver wifiDriver_;
    ConfigStore configStore_;
    WifiManager wifiManager_;
    ArduinoNtpClient ntpClient_;
    NtpManager ntpManager_;
    DeviceTypeRegistry deviceTypeRegistry_{DeviceTypeRegistry::withDefaults()};
    DeviceRegistryStore deviceRegistryStore_;
    DeviceRetainedDataStore retainedStateStore_;
    DeviceScopedDataStore deviceScopedDataStore_;
    DisplayLayoutStore displayLayoutStore_;
    DeviceEventDispatcher deviceEventDispatcher_{};
#if defined(ARDUINO)
    EspRandomDeviceIdSource deviceIdSource_;
#else
    SequentialDeviceIdSource deviceIdSource_;
#endif
    DeviceRegistry deviceRegistry_;
    RtcSyncCoordinator rtcSyncCoordinator_;
    DisplayRenderCoordinator displayRenderCoordinator_;
    DashboardLayoutStore dashboardLayoutStore_;
    LittleFsDoseJournalStorage doseJournalStorage_;
    SegmentedDoseJournal doseJournal_{doseJournalStorage_, kDoseJournalRecordsPerSegment};
    DoseJournalCleanupSink doseJournalCleanupSink_{doseJournal_};
    LittleFsSchedulePresetStorage schedulePresetStorage_;
    SchedulePresetCleanupSink schedulePresetCleanupSink_{schedulePresetStorage_};
    PortalServer portalServer_;
    uint32_t lastTick100ms_{0};
    uint32_t lastTick1s_{0};
    bool begun_{false};
#if defined(WITH_ARDUINO_OTA)
    ArduinoOtaService otaService_;
#endif
#if defined(WITH_HOME_ASSISTANT)
    PreferencesConfigStorage mqttConfigStorage_;
    MqttConfigStore mqttConfigStore_{mqttConfigStorage_};
    MqttManager mqttManager_;
    HaDiscoveryBridge haDiscoveryBridge_{&mqttManager_, &deviceRegistry_, &deviceEventDispatcher_, &deviceScopedDataStore_};
    ArduinoSystemStats systemStats_;
    SystemHaPublisher systemHaPublisher_{&mqttManager_, &wifiManager_, &systemStats_, &deviceRegistry_};
#endif
};

} // namespace ewfm
