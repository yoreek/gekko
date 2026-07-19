#include "devices/registry/DeviceRegistryBinaryCodec.h"

#include <cstring>
#include <unity.h>

using namespace ewfm;

namespace {

DeviceRegistryEntry makeRecord(DeviceId id, DeviceTypeId typeId, const DeviceConfigBlob& payload) {
    DeviceRegistryEntry record{};
    record.header.recordVersion = kDeviceRecordHeaderVersion;
    record.header.deviceId = id;
    record.header.typeId = typeId;
    record.header.configVersion = 2;
    record.header.configRevision = 7;
    record.header.payloadLength = static_cast<uint32_t>(payload.size());
    record.header.payloadChecksum = 0;
    record.depCount = 0;
    record.status = DeviceStatus::Ready;
    record.persistencePolicy = DevicePersistencePolicy::Delayed;
    return record;
}

DeviceConfigBlob makePayload(const char* value) {
    DeviceConfigBlob payload{};
    TEST_ASSERT_TRUE(payload.assign(reinterpret_cast<const uint8_t*>(value), std::strlen(value)));
    return payload;
}

} // namespace

void test_codec_hex_round_trip() {
    const std::string source = "\x01\x02\xA3\xff";
    const std::string hex = DeviceRegistryBinaryCodec::toHex(source);
    TEST_ASSERT_EQUAL_STRING("0102a3ff", hex.c_str());

    std::string restored;
    TEST_ASSERT_TRUE(DeviceRegistryBinaryCodec::fromHex(hex, restored));
    TEST_ASSERT_EQUAL_UINT32(source.size(), restored.size());
    TEST_ASSERT_EQUAL_MEMORY(source.data(), restored.data(), source.size());
}

void test_codec_index_round_trip() {
    DeviceRegistrySnapshot snapshot{};
    snapshot.indexEntries.push_back({101, 1});
    snapshot.indexEntries.push_back({102, 2});

    const std::string blob = DeviceRegistryBinaryCodec::serializeIndex(snapshot);
    DeviceRegistrySnapshot decoded{};
    DeviceValidationResult result = DeviceRegistryBinaryCodec::parseIndex(blob, decoded);
    TEST_ASSERT_TRUE(result.ok());
    TEST_ASSERT_EQUAL_UINT32(2, decoded.indexEntries.size());
    TEST_ASSERT_EQUAL_UINT32(101, decoded.indexEntries[0].deviceId);
    TEST_ASSERT_EQUAL_UINT32(2, decoded.indexEntries[1].typeId);
}

void test_codec_record_round_trip() {
    DeviceConfigBlob configBlob = makePayload("payload-v1");
    DeviceRegistryEntry record = makeRecord(501, 7, configBlob);
    record.depCount = 2;
    record.deps[0] = DeviceDependencyLink{DeviceRole::OneWireBus, 500, false};
    record.deps[1] = DeviceDependencyLink{DeviceRole::Condition, 600, true};
    record.status = DeviceStatus::DependencyBlocked;
    record.persistencePolicy = DevicePersistencePolicy::Coalesced;

    const std::string blob = DeviceRegistryBinaryCodec::serializeRecord(record, configBlob);
    DeviceRegistryEntry decoded{};
    DeviceConfigBlob decodedConfigBlob{};
    DeviceValidationResult result = DeviceRegistryBinaryCodec::parseRecord(blob, decoded, decodedConfigBlob);
    TEST_ASSERT_TRUE(result.ok());
    TEST_ASSERT_EQUAL_UINT32(record.header.deviceId, decoded.header.deviceId);
    TEST_ASSERT_EQUAL_UINT32(record.header.typeId, decoded.header.typeId);
    TEST_ASSERT_EQUAL_UINT32(record.header.configVersion, decoded.header.configVersion);
    TEST_ASSERT_EQUAL_UINT32(record.header.configRevision, decoded.header.configRevision);
    TEST_ASSERT_EQUAL_UINT32(configBlob.size(), decoded.header.payloadLength);
    TEST_ASSERT_EQUAL_UINT8(record.depCount, decoded.depCount);
    TEST_ASSERT_EQUAL(static_cast<int>(record.deps[0].role), static_cast<int>(decoded.deps[0].role));
    TEST_ASSERT_EQUAL_UINT32(record.deps[0].deviceId, decoded.deps[0].deviceId);
    TEST_ASSERT_FALSE(decoded.deps[0].invert);
    TEST_ASSERT_EQUAL(static_cast<int>(record.deps[1].role), static_cast<int>(decoded.deps[1].role));
    TEST_ASSERT_EQUAL_UINT32(record.deps[1].deviceId, decoded.deps[1].deviceId);
    TEST_ASSERT_TRUE(decoded.deps[1].invert);
    TEST_ASSERT_EQUAL(static_cast<int>(record.status), static_cast<int>(decoded.status));
    TEST_ASSERT_EQUAL(static_cast<int>(record.persistencePolicy), static_cast<int>(decoded.persistencePolicy));
    TEST_ASSERT_EQUAL_UINT32(configBlob.size(), decodedConfigBlob.size());
    TEST_ASSERT_EQUAL_MEMORY(configBlob.data(), decodedConfigBlob.data(), configBlob.size());
}

void test_codec_record_pointer_buffer_round_trips_dependency_invert_flag() {
    // Exercises the fixed-buffer serializeRecord()/parseRecord() overloads separately from the
    // std::string/vector ones above, since they're independent implementations.
    DeviceConfigBlob configBlob = makePayload("payload-v2");
    DeviceRegistryEntry record = makeRecord(502, 8, configBlob);
    record.depCount = 1;
    record.deps[0] = DeviceDependencyLink{DeviceRole::Condition, 601, true};

    uint8_t buffer[512]{};
    const size_t size = DeviceRegistryBinaryCodec::serializeRecord(record, configBlob, buffer, sizeof(buffer));
    TEST_ASSERT_TRUE(size > 0);

    DeviceRegistryEntry decoded{};
    DeviceConfigBlob decodedConfigBlob{};
    const DeviceValidationResult result = DeviceRegistryBinaryCodec::parseRecord(buffer, size, decoded, decodedConfigBlob);
    TEST_ASSERT_TRUE(result.ok());
    TEST_ASSERT_EQUAL_UINT8(record.depCount, decoded.depCount);
    TEST_ASSERT_EQUAL(static_cast<int>(record.deps[0].role), static_cast<int>(decoded.deps[0].role));
    TEST_ASSERT_EQUAL_UINT32(record.deps[0].deviceId, decoded.deps[0].deviceId);
    TEST_ASSERT_TRUE(decoded.deps[0].invert);
}

void test_codec_record_rejects_trailing_data() {
    const DeviceConfigBlob configBlob = makePayload("abcdef");
    const DeviceRegistryEntry record = makeRecord(701, 9, configBlob);
    std::string blob = DeviceRegistryBinaryCodec::serializeRecord(record, configBlob);
    TEST_ASSERT_TRUE(blob.size() > 0);
    blob.push_back('\0');
    DeviceRegistryEntry decoded{};
    DeviceConfigBlob decodedConfigBlob{};
    DeviceValidationResult result = DeviceRegistryBinaryCodec::parseRecord(blob, decoded, decodedConfigBlob);
    TEST_ASSERT_FALSE(result.ok());
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceError::CorruptRecord), static_cast<int>(result.error));
}
