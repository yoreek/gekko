#include "platform/ArduinoWifiDriver.h"

#if defined(ARDUINO) && !defined(UNIT_TEST)
#include <WiFi.h>
#endif

namespace ewfm {

bool ArduinoWifiDriver::beginStation(const WiFiCredentials& credentials) {
#if defined(ARDUINO) && !defined(UNIT_TEST)
    if (WiFi.status() == WL_CONNECTED) {
        return true;
    }

    WiFi.mode(WIFI_STA);
    WiFi.begin(credentials.ssid.c_str(), credentials.password.c_str());
    return true;
#else
    (void)credentials;
    return false;
#endif
}

void ArduinoWifiDriver::disconnect() {
#if defined(ARDUINO) && !defined(UNIT_TEST)
    WiFi.disconnect(false, false);
#endif
}

void ArduinoWifiDriver::clearStationCredentials() {
#if defined(ARDUINO) && !defined(UNIT_TEST)
    WiFi.disconnect(true, true);
#endif
}

bool ArduinoWifiDriver::startSetupAp(const std::string& ssid, const std::string& password) {
#if defined(ARDUINO) && !defined(UNIT_TEST)
    WiFi.mode(WIFI_AP_STA);
    setupApActive_ = true;
    if (password.empty()) {
        return WiFi.softAP(ssid.c_str());
    }
    return WiFi.softAP(ssid.c_str(), password.c_str());
#else
    (void)ssid;
    (void)password;
    return false;
#endif
}

void ArduinoWifiDriver::stopSetupAp() {
#if defined(ARDUINO) && !defined(UNIT_TEST)
    WiFi.softAPdisconnect(true);
    setupApActive_ = false;
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

bool ArduinoWifiDriver::setupApActive() const {
#if defined(ARDUINO) && !defined(UNIT_TEST)
    return setupApActive_;
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
    WiFi.scanDelete();
    return WiFi.scanNetworks(true, true) == WIFI_SCAN_RUNNING;
#else
    return false;
#endif
}

bool ArduinoWifiDriver::scanComplete(std::vector<WifiNetwork>& networks, size_t maxResults) {
#if defined(ARDUINO) && !defined(UNIT_TEST)
    int16_t count = WiFi.scanComplete();
    if (count < 0) {
        return false;
    }
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

} // namespace ewfm
