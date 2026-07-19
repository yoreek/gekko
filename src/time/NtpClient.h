#pragma once

#include <cstdint>
#include <string>

namespace ewfm {

enum class NtpResolveStatus {
    Idle,
    Pending,
    Resolved,
    Failed,
};

// Raw non-blocking NTP transport, mirroring IWifiDriver's interface/impl split: NtpManager (the
// state machine) depends only on this interface, so it stays unit-testable on env:native against
// a fake, while ArduinoNtpClient holds the real WiFiUDP socket + async DNS resolution. DNS
// resolution is folded into this interface (unlike raw dns_gethostbyname() callbacks) so
// NtpManager never touches lwIP/WiFiUDP directly.
class INtpClient {
public:
    virtual ~INtpClient() = default;

    virtual void beginResolve(const std::string& hostname) = 0;
    [[nodiscard]] virtual NtpResolveStatus resolveStatus() const = 0;

    virtual bool openSocket() = 0;
    virtual void closeSocket() = 0;
    virtual bool sendRequest() = 0;

    // Non-blocking poll; never blocks. Returns true and fills outEpochSeconds (UTC Unix epoch)
    // only when a fresh valid NTP reply has arrived since the last sendRequest().
    virtual bool pollResponse(uint32_t& outEpochSeconds) = 0;
};

} // namespace ewfm
