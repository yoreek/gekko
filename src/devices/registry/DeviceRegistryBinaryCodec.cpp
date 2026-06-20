#include "devices/registry/DeviceRegistryBinaryCodec.h"

#include <cstdio>
#include <cstring>
#include <type_traits>
#include <vector>

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

template <typename T> void appendLE(std::vector<uint8_t>& out, T value) {
    using Unsigned = typename std::make_unsigned<T>::type;
    const Unsigned v = static_cast<Unsigned>(value);
    for (size_t index = 0; index < sizeof(T); ++index) {
        out.push_back(static_cast<uint8_t>((v >> (index * 8)) & 0xFFU));
    }
}

template <typename T> bool appendLE(uint8_t* out, size_t capacity, size_t& pos, T value) {
    using Unsigned = typename std::make_unsigned<T>::type;
    if (pos + sizeof(T) > capacity) {
        return false;
    }
    const Unsigned v = static_cast<Unsigned>(value);
    for (size_t index = 0; index < sizeof(T); ++index) {
        out[pos + index] = static_cast<uint8_t>((v >> (index * 8)) & 0xFFU);
    }
    pos += sizeof(T);
    return true;
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

template <typename T> bool readLE(const std::vector<uint8_t>& blob, size_t& pos, T& value) {
    using Unsigned = typename std::make_unsigned<T>::type;
    if (pos + sizeof(T) > blob.size()) {
        return false;
    }
    Unsigned v{0};
    for (size_t index = 0; index < sizeof(T); ++index) {
        v |= static_cast<Unsigned>(blob[pos + index]) << (index * 8);
    }
    value = static_cast<T>(v);
    pos += sizeof(T);
    return true;
}

template <typename T> bool readLE(const uint8_t* blob, size_t size, size_t& pos, T& value) {
    using Unsigned = typename std::make_unsigned<T>::type;
    if (pos + sizeof(T) > size) {
        return false;
    }
    Unsigned v{0};
    for (size_t index = 0; index < sizeof(T); ++index) {
        v |= static_cast<Unsigned>(blob[pos + index]) << (index * 8);
    }
    value = static_cast<T>(v);
    pos += sizeof(T);
    return true;
}

void appendBytes(std::vector<uint8_t>& out, const uint8_t* bytes, size_t size) {
    out.insert(out.end(), bytes, bytes + size);
}

bool appendBytes(uint8_t* out, size_t capacity, size_t& pos, const uint8_t* bytes, size_t size) {
    if (pos + size > capacity) {
        return false;
    }
    std::memcpy(out + pos, bytes, size);
    pos += size;
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

bool readBytes(const uint8_t* blob, size_t size, size_t& pos, size_t length, BoundedBlob<kMaxDeviceConfigBytes>& out) {
    if (pos + length > size) {
        return false;
    }
    const bool ok = out.assign(blob + pos, length);
    pos += length;
    return ok;
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

std::string DeviceRegistryBinaryCodec::serializeRecord(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob) {
    const std::vector<uint8_t> blob = serializeRecordBlob(record, configBlob);
    return std::string(blob.begin(), blob.end());
}

std::vector<uint8_t> DeviceRegistryBinaryCodec::serializeRecordBlob(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob) {
    const uint8_t depCount = record.dependencyCount();
    const DeviceDependencyLink* dependencyLinks = record.dependencyLinks();
    std::vector<uint8_t> blob;
    blob.reserve(48 + configBlob.size() + static_cast<size_t>(depCount) * 8U);
    appendLE<uint32_t>(blob, kRecordMagic);
    appendLE<uint16_t>(blob, kDeviceRecordHeaderVersion);
    appendLE<uint16_t>(blob, 0);
    appendLE<DeviceId>(blob, record.header.deviceId);
    appendLE<DeviceTypeId>(blob, record.header.typeId);
    appendLE<uint16_t>(blob, 0U);
    appendLE<uint32_t>(blob, record.header.configVersion);
    appendLE<uint32_t>(blob, record.header.configRevision);
    appendLE<uint32_t>(blob, static_cast<uint32_t>(configBlob.size()));
    appendLE<uint32_t>(blob, record.header.payloadChecksum);
    appendLE<uint8_t>(blob, depCount);
    appendLE<uint8_t>(blob, static_cast<uint8_t>(record.status));
    appendLE<uint8_t>(blob, static_cast<uint8_t>(record.persistencePolicy));
    appendLE<uint8_t>(blob, 0U);
    for (uint8_t index = 0; index < depCount && index < kMaxDeviceDependencies && dependencyLinks != nullptr; ++index) {
        appendLE<uint8_t>(blob, static_cast<uint8_t>(dependencyLinks[index].role));
        appendLE<uint8_t>(blob, 0U);
        appendLE<uint16_t>(blob, 0U);
        appendLE<DeviceId>(blob, dependencyLinks[index].deviceId);
    }
    appendBytes(blob, configBlob.data(), configBlob.size());
    return blob;
}

size_t DeviceRegistryBinaryCodec::serializeRecord(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob, uint8_t* out,
                                                  size_t capacity) {
    size_t pos = 0;
    if (!appendLE<uint32_t>(out, capacity, pos, kRecordMagic) || !appendLE<uint16_t>(out, capacity, pos, kDeviceRecordHeaderVersion) ||
        !appendLE<uint16_t>(out, capacity, pos, 0) || !appendLE<DeviceId>(out, capacity, pos, record.header.deviceId) ||
        !appendLE<DeviceTypeId>(out, capacity, pos, record.header.typeId) || !appendLE<uint16_t>(out, capacity, pos, 0U) ||
        !appendLE<uint32_t>(out, capacity, pos, record.header.configVersion) ||
        !appendLE<uint32_t>(out, capacity, pos, record.header.configRevision) ||
        !appendLE<uint32_t>(out, capacity, pos, static_cast<uint32_t>(configBlob.size())) ||
        !appendLE<uint32_t>(out, capacity, pos, record.header.payloadChecksum) ||
        !appendLE<uint8_t>(out, capacity, pos, record.dependencyCount()) ||
        !appendLE<uint8_t>(out, capacity, pos, static_cast<uint8_t>(record.status)) ||
        !appendLE<uint8_t>(out, capacity, pos, static_cast<uint8_t>(record.persistencePolicy)) ||
        !appendLE<uint8_t>(out, capacity, pos, 0U)) {
        return 0U;
    }
    const DeviceDependencyLink* dependencyLinks = record.dependencyLinks();
    for (uint8_t index = 0; index < record.dependencyCount() && index < kMaxDeviceDependencies && dependencyLinks != nullptr; ++index) {
        if (!appendLE<uint8_t>(out, capacity, pos, static_cast<uint8_t>(dependencyLinks[index].role)) ||
            !appendLE<uint8_t>(out, capacity, pos, 0U) || !appendLE<uint16_t>(out, capacity, pos, 0U) ||
            !appendLE<DeviceId>(out, capacity, pos, dependencyLinks[index].deviceId)) {
            return 0U;
        }
    }
    if (!appendBytes(out, capacity, pos, configBlob.data(), configBlob.size())) {
        return 0U;
    }
    return pos;
}

DeviceValidationResult DeviceRegistryBinaryCodec::parseRecord(const std::string& blob, DeviceRegistryEntry& record,
                                                              DeviceConfigBlob& configBlob) {
    std::vector<uint8_t> bytes(blob.begin(), blob.end());
    return parseRecordBlob(bytes, record, configBlob);
}

DeviceValidationResult DeviceRegistryBinaryCodec::parseRecord(const uint8_t* blob, size_t size, DeviceRegistryEntry& record,
                                                              DeviceConfigBlob& configBlob) {
    size_t pos = 0;
    uint32_t magic{0};
    uint16_t version{0};
    uint16_t reserved16{0};
    uint16_t reservedRecordFlags{0};
    uint32_t payloadLength{0};
    uint8_t depCount{0};
    uint8_t status{0};
    uint8_t persistence{0};
    uint8_t reserved8{0};
    if (!readLE(blob, size, pos, magic) || !readLE(blob, size, pos, version) || !readLE(blob, size, pos, reserved16)) {
        return {DeviceError::CorruptRecord, "device record is truncated"};
    }
    if (magic != kRecordMagic || version != kDeviceRecordHeaderVersion) {
        return {DeviceError::InvalidVersion, "unsupported device record version"};
    }

    record = {};
    if (!readLE(blob, size, pos, record.header.deviceId) || !readLE(blob, size, pos, record.header.typeId) ||
        !readLE(blob, size, pos, reservedRecordFlags) || !readLE(blob, size, pos, record.header.configVersion) ||
        !readLE(blob, size, pos, record.header.configRevision) || !readLE(blob, size, pos, payloadLength) ||
        !readLE(blob, size, pos, record.header.payloadChecksum) || !readLE(blob, size, pos, depCount) || !readLE(blob, size, pos, status) ||
        !readLE(blob, size, pos, persistence) || !readLE(blob, size, pos, reserved8)) {
        return {DeviceError::CorruptRecord, "device record header is truncated"};
    }

    if (payloadLength > kMaxDeviceConfigBytes) {
        return {DeviceError::BoundsExceeded, "device record exceeds supported size"};
    }
    if (depCount > kMaxDeviceDependencies) {
        return {DeviceError::BoundsExceeded, "device record exceeds supported dependency count"};
    }

    record.header.recordVersion = version;
    record.depCount = depCount;
    for (uint8_t index = 0; index < record.depCount; ++index) {
        uint8_t role{0};
        uint8_t reservedLink8{0};
        uint16_t reservedLink16{0};
        DeviceDependencyLink link{};
        if (!readLE(blob, size, pos, role) || !readLE(blob, size, pos, reservedLink8) || !readLE(blob, size, pos, reservedLink16) ||
            !readLE(blob, size, pos, link.deviceId)) {
            return {DeviceError::CorruptRecord, "device record dependency data is truncated"};
        }
        link.role = static_cast<DeviceDependencyRole>(role);
        record.deps[index] = link;
        (void)reservedLink8;
        (void)reservedLink16;
    }

    if (pos + payloadLength > size) {
        return {DeviceError::CorruptRecord, "device record payload is truncated"};
    }
    if (!readBytes(blob, size, pos, payloadLength, configBlob)) {
        return {DeviceError::CorruptRecord, "device record payload is truncated"};
    }

    if (pos != size) {
        return {DeviceError::CorruptRecord, "device record has trailing data"};
    }

    (void)reserved16;
    (void)reservedRecordFlags;
    (void)reserved8;
    record.header.payloadLength = payloadLength;
    record.status = static_cast<DeviceStatus>(status);
    record.persistencePolicy = static_cast<DevicePersistencePolicy>(persistence);
    return {};
}

DeviceValidationResult DeviceRegistryBinaryCodec::parseRecordBlob(const std::vector<uint8_t>& blob, DeviceRegistryEntry& record,
                                                                  DeviceConfigBlob& configBlob) {
    return parseRecord(blob.data(), blob.size(), record, configBlob);
}

} // namespace ewfm
