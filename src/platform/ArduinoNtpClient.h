#pragma once

#include "time/NtpClient.h"

#include <cstdint>

#if defined(ARDUINO) && !defined(UNIT_TEST)
#include <WiFiUdp.h>
extern "C" {
#include "lwip/dns.h"
}
#endif

namespace ewfm {

// Concrete Arduino-backed INtpClient, ported from a previous project's non-blocking NTP client
// (WiFiUDP + async lwIP DNS resolution). Mirrors ArduinoWifiDriver: every method is always
// defined, but the real socket/DNS work only happens under ARDUINO && !UNIT_TEST; under native
// unit tests every call just fails/no-ops, matching the rest of the platform layer.
class ArduinoNtpClient final : public INtpClient {
public:
    void beginResolve(const std::string& hostname) override;
    [[nodiscard]] NtpResolveStatus resolveStatus() const override;

    bool openSocket() override;
    void closeSocket() override;
    bool sendRequest() override;
    bool pollResponse(uint32_t& outEpochSeconds) override;

private:
    NtpResolveStatus resolveStatus_{NtpResolveStatus::Idle};

#if defined(ARDUINO) && !defined(UNIT_TEST)
    struct NtpPacket {
        uint8_t liVnMode{0};
        uint8_t stratum{0};
        uint8_t poll{0};
        uint8_t precision{0};
        uint32_t rootDelay{0};
        uint32_t rootDispersion{0};
        uint32_t refId{0};
        uint32_t refTmS{0};
        uint32_t refTmF{0};
        uint32_t origTmS{0};
        uint32_t origTmF{0};
        uint32_t rxTmS{0};
        uint32_t rxTmF{0};
        uint32_t txTmS{0};
        uint32_t txTmF{0};
    };
    static_assert(sizeof(NtpPacket) == 48, "NTP packet must be exactly 48 bytes on the wire");

    static void dnsCallback(const char* name, const ip_addr_t* ipaddr, void* arg);
    void handleDnsResult(const ip_addr_t* ipaddr);
    void flushPackets();
    void sendNtpPacket();

    WiFiUDP udp_;
    IPAddress serverAddress_{};
#endif
};

} // namespace ewfm
