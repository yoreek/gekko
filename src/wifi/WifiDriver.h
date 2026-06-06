#pragma once

#include "config/DeviceConfig.h"

#include <cstdint>
#include <string>
#include <vector>

namespace ewfm {

enum class WifiDriverStatus {
    Idle,
    Connecting,
    Connected,
    Disconnected,
    Failed,
};

enum class WifiSecurity {
    Open,
    Wep,
    Wpa,
    Wpa2,
    WpaWpa2,
    Unknown,
};

struct WifiNetwork {
    std::string ssid;
    int32_t rssi{0};
    WifiSecurity security{WifiSecurity::Unknown};
    int32_t channel{0};
};

class IWifiDriver {
public:
    virtual ~IWifiDriver() = default;
    virtual bool begin() = 0;
    virtual bool beginStation(const WiFiCredentials& credentials) = 0;
    virtual void disconnect() = 0;
    virtual void clearStationCredentials() = 0;
    virtual bool startSetupAp(const std::string& ssid, const std::string& password) = 0;
    virtual void stopSetupAp() = 0;
    virtual WifiDriverStatus status() const = 0;
    virtual bool setupApActive() const = 0;
    virtual bool networkStackReady() const = 0;
    virtual bool stationReady() const = 0;
    virtual bool setupApReady() const = 0;
    virtual std::string stationIp() const = 0;
    virtual std::string setupApIp() const = 0;
    virtual bool startScan() = 0;
    virtual bool scanComplete(std::vector<WifiNetwork>& networks, size_t maxResults) = 0;
    virtual std::string macSuffix() const = 0;
};

} // namespace ewfm
