#include "platform/ArduinoWifiDriver.h"

#include "debug/Debug.h"

#if defined(ARDUINO) && !defined(UNIT_TEST)
#include <WiFi.h>
#endif

namespace ewfm {

#if defined(ARDUINO) && !defined(UNIT_TEST)
namespace {

const char* wifiModeName(wifi_mode_t mode) {
    switch (mode) {
    case WIFI_MODE_NULL:
        return "NULL";
    case WIFI_MODE_STA:
        return "STA";
    case WIFI_MODE_AP:
        return "AP";
    case WIFI_MODE_APSTA:
        return "AP_STA";
    default:
        return "UNKNOWN";
    }
}

const char* wifiStatusName(wl_status_t status) {
    switch (status) {
    case WL_IDLE_STATUS:
        return "IDLE";
    case WL_NO_SSID_AVAIL:
        return "NO_SSID_AVAIL";
    case WL_SCAN_COMPLETED:
        return "SCAN_COMPLETED";
    case WL_CONNECTED:
        return "CONNECTED";
    case WL_CONNECT_FAILED:
        return "CONNECT_FAILED";
    case WL_CONNECTION_LOST:
        return "CONNECTION_LOST";
    case WL_DISCONNECTED:
        return "DISCONNECTED";
    default:
        return "UNKNOWN";
    }
}

void logWifiSnapshot(const char* action) {
    const wifi_mode_t mode = WiFi.getMode();
    const wl_status_t status = WiFi.status();
    const std::string stationIp = WiFi.localIP().toString().c_str();
    const std::string setupApIp = WiFi.softAPIP().toString().c_str();
    EWFM_WIFI_LOG_INFO("%s mode=%s status=%s staIp=%s apIp=%s stackReady=%d apActive=%d", action, wifiModeName(mode),
                       wifiStatusName(status), stationIp.c_str(), setupApIp.c_str(), static_cast<int>(mode != WIFI_MODE_NULL),
                       static_cast<int>(WiFi.getMode() == WIFI_MODE_AP || WiFi.getMode() == WIFI_MODE_APSTA));
}

} // namespace
#endif

bool ArduinoWifiDriver::begin() {
#if defined(ARDUINO) && !defined(UNIT_TEST)
    WiFi.persistent(false);
    WiFi.setAutoReconnect(false);
    EWFM_WIFI_LOG_INFO("driver begin: mode=WIFI_STA");
    networkStackReady_ = WiFi.mode(WIFI_STA);
    EWFM_WIFI_LOG_INFO("driver begin: mode result=%d", static_cast<int>(networkStackReady_));
    logWifiSnapshot("driver begin");
    return networkStackReady_;
#else
    return false;
#endif
}

bool ArduinoWifiDriver::beginStation(const WiFiCredentials& credentials) {
#if defined(ARDUINO) && !defined(UNIT_TEST)
    EWFM_WIFI_LOG_INFO("beginStation ssid=%s", credentials.ssid.c_str());
    EWFM_WIFI_LOG_INFO("beginStation: mode=WIFI_STA");
    networkStackReady_ = WiFi.mode(WIFI_STA);
    if (!networkStackReady_) {
        EWFM_WIFI_LOG_WARN("beginStation: WiFi.mode(WIFI_STA) failed");
        logWifiSnapshot("beginStation mode failed");
        return false;
    }
    EWFM_WIFI_LOG_INFO("beginStation: WiFi.begin(ssid, ****)");
    WiFi.begin(credentials.ssid.c_str(), credentials.password.c_str());
    logWifiSnapshot("beginStation requested");
    return true;
#else
    (void)credentials;
    return false;
#endif
}

void ArduinoWifiDriver::disconnect() {
#if defined(ARDUINO) && !defined(UNIT_TEST)
    EWFM_WIFI_LOG_INFO("disconnect: WiFi.disconnect(false, false)");
    WiFi.disconnect(false, false);
    networkStackReady_ = WiFi.getMode() != WIFI_MODE_NULL;
    logWifiSnapshot("disconnect");
#endif
}

void ArduinoWifiDriver::clearStationCredentials() {
#if defined(ARDUINO) && !defined(UNIT_TEST)
    EWFM_WIFI_LOG_INFO("clearStationCredentials: WiFi.disconnect(true, true)");
    WiFi.disconnect(true, true);
    networkStackReady_ = WiFi.getMode() != WIFI_MODE_NULL;
    logWifiSnapshot("clearStationCredentials");
#endif
}

bool ArduinoWifiDriver::startSetupAp(const std::string& ssid, const std::string& password) {
#if defined(ARDUINO) && !defined(UNIT_TEST)
    EWFM_WIFI_LOG_INFO("startSetupAp ssid=%s password=%s", ssid.c_str(), password.empty() ? "<empty>" : "<set>");
    EWFM_WIFI_LOG_INFO("startSetupAp: mode=WIFI_AP_STA");
    networkStackReady_ = WiFi.mode(WIFI_AP_STA);
    if (!networkStackReady_) {
        EWFM_WIFI_LOG_WARN("startSetupAp: WiFi.mode(WIFI_AP_STA) failed");
        logWifiSnapshot("startSetupAp mode failed");
        return false;
    }

    bool started = password.empty() ? WiFi.softAP(ssid.c_str()) : WiFi.softAP(ssid.c_str(), password.c_str());
    EWFM_WIFI_LOG_INFO("startSetupAp: WiFi.softAP result=%d", static_cast<int>(started));
    logWifiSnapshot("startSetupAp");
    return started;
#else
    (void)ssid;
    (void)password;
    return false;
#endif
}

void ArduinoWifiDriver::stopSetupAp() {
#if defined(ARDUINO) && !defined(UNIT_TEST)
    EWFM_WIFI_LOG_INFO("stopSetupAp: WiFi.softAPdisconnect(true)");
    WiFi.softAPdisconnect(true);
    logWifiSnapshot("stopSetupAp");
#endif
}

WifiDriverStatus ArduinoWifiDriver::status() const {
#if defined(ARDUINO) && !defined(UNIT_TEST)
    switch (WiFi.status()) {
    case WL_CONNECTED:
        return WifiDriverStatus::Connected;
    case WL_CONNECT_FAILED:
    case WL_NO_SSID_AVAIL:
        return WifiDriverStatus::Failed;
    case WL_DISCONNECTED:
        return WifiDriverStatus::Disconnected;
    default:
        return WifiDriverStatus::Connecting;
    }
#else
    return WifiDriverStatus::Idle;
#endif
}

bool ArduinoWifiDriver::networkStackReady() const {
#if defined(ARDUINO) && !defined(UNIT_TEST)
    return networkStackReady_ && WiFi.getMode() != WIFI_MODE_NULL;
#else
    return false;
#endif
}

bool ArduinoWifiDriver::stationReady() const {
#if defined(ARDUINO) && !defined(UNIT_TEST)
    return networkStackReady() && WiFi.status() == WL_CONNECTED && ipValid(stationIp());
#else
    return false;
#endif
}

bool ArduinoWifiDriver::setupApReady() const {
#if defined(ARDUINO) && !defined(UNIT_TEST)
    return networkStackReady() && ipValid(setupApIp());
#else
    return false;
#endif
}

std::string ArduinoWifiDriver::stationIp() const {
#if defined(ARDUINO) && !defined(UNIT_TEST)
    return WiFi.localIP().toString().c_str();
#else
    return {};
#endif
}

std::string ArduinoWifiDriver::setupApIp() const {
#if defined(ARDUINO) && !defined(UNIT_TEST)
    return WiFi.softAPIP().toString().c_str();
#else
    return {};
#endif
}

bool ArduinoWifiDriver::startScan() {
#if defined(ARDUINO) && !defined(UNIT_TEST)
    EWFM_WIFI_LOG_INFO("startScan");
    WiFi.scanDelete();
    const bool started = WiFi.scanNetworks(true, true) == WIFI_SCAN_RUNNING;
    EWFM_WIFI_LOG_INFO("startScan result=%d", static_cast<int>(started));
    logWifiSnapshot("startScan");
    return started;
#else
    return false;
#endif
}

bool ArduinoWifiDriver::scanComplete(std::vector<WifiNetwork>& networks, size_t maxResults) {
#if defined(ARDUINO) && !defined(UNIT_TEST)
    EWFM_WIFI_LOG_INFO("scanComplete requested maxResults=%u", static_cast<unsigned>(maxResults));
    int16_t count = WiFi.scanComplete();
    if (count == WIFI_SCAN_RUNNING) {
        EWFM_WIFI_LOG_DEBUG("scanComplete not ready count=%d", static_cast<int>(count));
        return false;
    }
    if (count == WIFI_SCAN_FAILED) {
        EWFM_WIFI_LOG_WARN("scanComplete failed");
        WiFi.scanDelete();
        networks.clear();
        logWifiSnapshot("scanComplete failed");
        return true;
    }
    EWFM_WIFI_LOG_INFO("scanComplete count=%d", static_cast<int>(count));
    networks.clear();
    const size_t limit = static_cast<size_t>(count) < maxResults ? static_cast<size_t>(count) : maxResults;
    for (size_t i = 0; i < limit; ++i) {
        WifiNetwork network;
        network.ssid = WiFi.SSID(i).c_str();
        network.rssi = WiFi.RSSI(i);
        network.channel = WiFi.channel(i);
        network.security = WiFi.encryptionType(i) == WIFI_AUTH_OPEN ? WifiSecurity::Open : WifiSecurity::Unknown;
        networks.push_back(network);
    }
    WiFi.scanDelete();
    logWifiSnapshot("scanComplete");
    return true;
#else
    (void)networks;
    (void)maxResults;
    return false;
#endif
}

std::string ArduinoWifiDriver::macSuffix() const {
#if defined(ARDUINO) && !defined(UNIT_TEST)
    String mac = WiFi.macAddress();
    mac.replace(":", "");
    if (mac.length() > 6) {
        mac = mac.substring(mac.length() - 6);
    }
    return mac.c_str();
#else
    return {};
#endif
}

int32_t ArduinoWifiDriver::rssi() const {
#if defined(ARDUINO) && !defined(UNIT_TEST)
    return WiFi.RSSI();
#else
    return 0;
#endif
}

std::string ArduinoWifiDriver::ssid() const {
#if defined(ARDUINO) && !defined(UNIT_TEST)
    return WiFi.SSID().c_str();
#else
    return {};
#endif
}

bool ArduinoWifiDriver::ipValid(const std::string& ip) {
    return !ip.empty() && ip != "0.0.0.0";
}

} // namespace ewfm
