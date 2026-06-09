#include "devices/registry/DeviceRegistryBinaryCodec.h"

#include <cstdio>
#include <cstring>
#include <type_traits>

namespace ewfm {

namespace {
constexpr uint32_t kRegistryMagic = 0x44565249UL;
constexpr uint32_t kRecordMagic = 0x44565243UL;
constexpr char kHexDigits[] = "0123456789abcdef";

template <typename T> void appendLE(std::string& out, T value) {
    using Unsigned = typename std::make_unsigned<T>::type;
    const Unsigned v = static_cast<Unsigned>(value);
    for (size_t index = 0; index < sizeof(T); ++index) {
        out.push_back(static_cast<char>((v >> (index * 8)) & 0xFFU));
    }
}

template <typename T> bool readLE(const std::string& blob, size_t& pos, T& value) {
    using Unsigned = typename std::make_unsigned<T>::type;
    if (pos + sizeof(T) > blob.size()) {
        return false;
    }
    Unsigned v{0};
    for (size_t index = 0; index < sizeof(T); ++index) {
        v |= static_cast<Unsigned>(static_cast<unsigned char>(blob[pos + index])) << (index * 8);
    }
    value = static_cast<T>(v);
    pos += sizeof(T);
    return true;
}

void appendBytes(std::string& out, const std::string& bytes) {
    out.append(bytes.data(), bytes.size());
}

bool readBytes(const std::string& blob, size_t& pos, size_t length, std::string& out) {
    if (pos + length > blob.size()) {
        return false;
    }
    out.assign(blob.data() + pos, length);
    pos += length;
    return true;
}

int fromHexDigit(char ch) {
    if (ch >= '0' && ch <= '9') {
        return ch - '0';
    }
    if (ch >= 'a' && ch <= 'f') {
        return 10 + (ch - 'a');
    }
    if (ch >= 'A' && ch <= 'F') {
        return 10 + (ch - 'A');
    }
    return -1;
}
} // namespace

std::string DeviceRegistryBinaryCodec::makeRecordKey(DeviceId deviceId) {
    char buffer[32];
    std::snprintf(buffer, sizeof(buffer), "record_%08x", static_cast<unsigned>(deviceId));
    return buffer;
}

bool DeviceRegistryBinaryCodec::fromHex(const std::string& hex, std::string& bytes) {
    if ((hex.size() % 2) != 0) {
        return false;
    }
    bytes.clear();
    bytes.reserve(hex.size() / 2);
    for (size_t index = 0; index < hex.size(); index += 2) {
        const int hi = fromHexDigit(hex[index]);
        const int lo = fromHexDigit(hex[index + 1]);
        if (hi < 0 || lo < 0) {
            return false;
        }
        bytes.push_back(static_cast<char>((hi << 4) | lo));
    }
    return true;
}

std::string DeviceRegistryBinaryCodec::toHex(const std::string& bytes) {
    std::string hex;
    hex.reserve(bytes.size() * 2);
    for (unsigned char byte : bytes) {
        hex.push_back(kHexDigits[(byte >> 4) & 0x0F]);
        hex.push_back(kHexDigits[byte & 0x0F]);
    }
    return hex;
}

uint32_t DeviceRegistryBinaryCodec::payloadChecksum(const std::string& payload) {
    uint32_t hash = 2166136261UL;
    for (unsigned char byte : payload) {
        hash ^= byte;
        hash *= 16777619UL;
    }
    return hash;
}

std::string DeviceRegistryBinaryCodec::serializeIndex(const DeviceRegistrySnapshot& snapshot) {
    std::string blob;
    blob.reserve(16 + snapshot.indexEntries.size() * 8);
    appendLE<uint32_t>(blob, kRegistryMagic);
    appendLE<uint16_t>(blob, kDeviceRegistryIndexVersion);
    appendLE<uint16_t>(blob, static_cast<uint16_t>(snapshot.indexEntries.size()));
    for (const auto& entry : snapshot.indexEntries) {
        appendLE<uint32_t>(blob, entry.deviceId);
        appendLE<uint16_t>(blob, entry.typeId);
        appendLE<uint16_t>(blob, 0);
    }
    return blob;
}

DeviceValidationResult DeviceRegistryBinaryCodec::parseIndex(const std::string& blob, DeviceRegistrySnapshot& snapshot) {
    size_t pos = 0;
    uint32_t magic{0};
    uint16_t version{0};
    uint16_t count{0};
    if (!readLE(blob, pos, magic) || !readLE(blob, pos, version) || !readLE(blob, pos, count)) {
        return {DeviceError::CorruptRecord, "registry index is truncated"};
    }
    if (magic != kRegistryMagic || version != kDeviceRegistryIndexVersion) {
        return {DeviceError::InvalidVersion, "unsupported registry index version"};
    }
    if (count > kMaxDynamicDevices) {
        return {DeviceError::BoundsExceeded, "registry index exceeds supported device count"};
    }

    snapshot.indexEntries.clear();
    snapshot.indexEntries.reserve(count);
    for (uint16_t i = 0; i < count; ++i) {
        DeviceIndexEntry entry{};
        uint16_t reserved{0};
        if (!readLE(blob, pos, entry.deviceId) || !readLE(blob, pos, entry.typeId) || !readLE(blob, pos, reserved)) {
            return {DeviceError::CorruptRecord, "registry index entry is truncated"};
        }
        snapshot.indexEntries.push_back(entry);
    }

    if (pos != blob.size()) {
        return {DeviceError::CorruptRecord, "registry index has trailing data"};
    }
    return {};
}

std::string DeviceRegistryBinaryCodec::serializeRecord(const DeviceRecord& record) {
    const uint32_t payloadLength =
        record.configPayload.size() > kMaxDeviceConfigBytes ? 0U : static_cast<uint32_t>(record.configPayload.size());
    std::string blob;
    blob.reserve(48 + record.name.size() + record.configPayload.size());
    appendLE<uint32_t>(blob, kRecordMagic);
    appendLE<uint16_t>(blob, kDeviceRecordHeaderVersion);
    appendLE<uint16_t>(blob, 0);
    appendLE<DeviceId>(blob, record.header.deviceId);
    appendLE<DeviceTypeId>(blob, record.header.typeId);
    appendLE<uint16_t>(blob, record.enabled ? 1U : 0U);
    appendLE<uint32_t>(blob, record.header.configVersion);
    appendLE<uint32_t>(blob, record.header.configRevision);
    appendLE<uint32_t>(blob, payloadLength);
    appendLE<uint32_t>(blob, payloadChecksum(record.configPayload));
    appendLE<uint32_t>(blob, static_cast<uint32_t>(record.name.size()));
    appendLE<uint8_t>(blob, record.hasParent ? 1U : 0U);
    appendLE<uint8_t>(blob, static_cast<uint8_t>(record.status));
    appendLE<uint8_t>(blob, static_cast<uint8_t>(record.persistencePolicy));
    appendLE<uint8_t>(blob, 0U);
    appendLE<DeviceId>(blob, record.parentDeviceId);
    appendBytes(blob, record.name);
    appendBytes(blob, record.configPayload);
    return blob;
}

DeviceValidationResult DeviceRegistryBinaryCodec::parseRecord(const std::string& blob, DeviceRecord& record) {
    size_t pos = 0;
    uint32_t magic{0};
    uint16_t version{0};
    uint16_t reserved16{0};
    uint16_t enabled{0};
    uint32_t nameLength{0};
    uint8_t hasParent{0};
    uint8_t status{0};
    uint8_t persistence{0};
    uint8_t reserved8{0};
    if (!readLE(blob, pos, magic) || !readLE(blob, pos, version) || !readLE(blob, pos, reserved16)) {
        return {DeviceError::CorruptRecord, "device record is truncated"};
    }
    if (magic != kRecordMagic || version != kDeviceRecordHeaderVersion) {
        return {DeviceError::InvalidVersion, "unsupported device record version"};
    }

    if (!readLE(blob, pos, record.header.deviceId) || !readLE(blob, pos, record.header.typeId) || !readLE(blob, pos, enabled) ||
        !readLE(blob, pos, record.header.configVersion) || !readLE(blob, pos, record.header.configRevision) ||
        !readLE(blob, pos, record.header.payloadLength) || !readLE(blob, pos, record.header.payloadChecksum) ||
        !readLE(blob, pos, nameLength) || !readLE(blob, pos, hasParent) || !readLE(blob, pos, status) || !readLE(blob, pos, persistence) ||
        !readLE(blob, pos, reserved8) || !readLE(blob, pos, record.parentDeviceId)) {
        return {DeviceError::CorruptRecord, "device record header is truncated"};
    }

    if (nameLength > kMaxDynamicDeviceNameLength || record.header.payloadLength > kMaxDeviceConfigBytes) {
        return {DeviceError::BoundsExceeded, "device record exceeds supported size"};
    }

    if (!readBytes(blob, pos, nameLength, record.name) || !readBytes(blob, pos, record.header.payloadLength, record.configPayload)) {
        return {DeviceError::CorruptRecord, "device record payload is truncated"};
    }

    if (pos != blob.size()) {
        return {DeviceError::CorruptRecord, "device record has trailing data"};
    }

    if (record.header.payloadChecksum != payloadChecksum(record.configPayload)) {
        return {DeviceError::InvalidConfig, "device record payload checksum mismatch"};
    }

    record.header.recordVersion = version;
    record.enabled = enabled != 0;
    record.hasParent = hasParent != 0;
    record.status = static_cast<DeviceStatus>(status);
    record.persistencePolicy = static_cast<DevicePersistencePolicy>(persistence);
    return {};
}

} // namespace ewfm
