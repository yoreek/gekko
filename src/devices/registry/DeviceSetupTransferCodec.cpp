#include "devices/registry/DeviceSetupTransferCodec.h"

#include "devices/core/DeviceBaseConfig.h"
#include "devices/bus/onewire/OneWireBusConfig.h"
#include "devices/bus/onewire/OneWireBusDevice.h"
#include "devices/dummy/DummyDevice.h"
#include "devices/sensors/ds18b20/Ds18b20TemperatureSensorConfig.h"
#include "devices/sensors/ds18b20/Ds18b20TemperatureSensorDevice.h"
#include "devices/switch/gpio/GpioSwitchDevice.h"
#include "devices/thermostat/ThermostatDeviceConfig.h"
#include "devices/thermostat/ThermostatDevice.h"

#include <cstdio>
#include <cstring>
#include <vector>

#if defined(ARDUINO) && !defined(UNIT_TEST)
#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#else
#include <ArduinoJson.h>
#include <fstream>
#endif

namespace ewfm {

namespace {
constexpr char kRecordKindEnvelope[] = "transfer_envelope";
constexpr char kRecordKindDevice[] = "device";

template <typename Writer> bool writeLine(Writer& writer, const std::string& line) {
    if (line.empty()) {
        return true;
    }
    return writer.write(reinterpret_cast<const uint8_t*>(line.data()), line.size()) == line.size() &&
           writer.write(reinterpret_cast<const uint8_t*>("\n"), 1U) == 1U;
}

struct StreamWriter {
#if defined(ARDUINO) && !defined(UNIT_TEST)
    AsyncResponseStream& stream;
    size_t write(const uint8_t* data, size_t size) {
        return stream.write(data, size);
    }
#endif
};

struct StringWriter {
    std::string& out;
    size_t write(const uint8_t* data, size_t size) {
        out.append(reinterpret_cast<const char*>(data), size);
        return size;
    }
};

const char* deviceTypeNameFromId(const DeviceTypeId typeId) {
    switch (typeId) {
    case 1:
        return "dummy";
    case 2:
        return "gpio_switch";
    case 3:
        return "onewire_bus";
    case 4:
        return "ds18b20_temperature_sensor";
    case 5:
        return "thermostat";
    default:
        return "unknown";
    }
}

bool deviceTypeIdFromTransferJson(const JsonObjectConst& input, DeviceTypeId& typeId, std::string& error) {
    const uint32_t numericTypeId = input["typeId"] | (input["type_id"] | 0U);
    if (numericTypeId != 0U) {
        typeId = static_cast<DeviceTypeId>(numericTypeId);
        return true;
    }

    const char* typeName = input["typeName"] | (input["type"] | "");
    if (std::strcmp(typeName, "dummy") == 0) {
        typeId = 1U;
        return true;
    }
    if (std::strcmp(typeName, "gpio_switch") == 0) {
        typeId = 2U;
        return true;
    }
    if (std::strcmp(typeName, "onewire_bus") == 0) {
        typeId = 3U;
        return true;
    }
    if (std::strcmp(typeName, "ds18b20_temperature_sensor") == 0) {
        typeId = 4U;
        return true;
    }
    if (std::strcmp(typeName, "thermostat") == 0) {
        typeId = 5U;
        return true;
    }

    error = "device bundle type is missing or invalid";
    return false;
}

bool encodeTransferConfigBlob(const JsonObjectConst& input, const DeviceBaseConfigV1& base, const DeviceTypeId typeId,
                              const uint32_t configVersion, DeviceConfigBlob& configBlob, std::string& error) {
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    size_t size = 0U;

    switch (typeId) {
    case 1: {
        DummyDeviceConfigV1 config = base;
        const char* parseError = nullptr;
        if (!parseDummyDeviceConfigJson(input, configVersion, config, parseError)) {
            error = parseError != nullptr ? parseError : "dummy device config is invalid";
            return false;
        }
        size = dummyDeviceConfigSize(config);
        if (!encodeDummyDeviceConfig(config, buffer, size)) {
            error = "dummy device config is invalid";
            return false;
        }
        break;
    }
    case 2: {
        if (configVersion != 1U) {
            error = "unsupported gpio switch config version";
            return false;
        }
        GpioSwitchDevicePersistedConfigV1 config{};
        const char* parseError = nullptr;
        if (!parseGpioSwitchDeviceConfigJson(input, config, parseError)) {
            error = parseError != nullptr ? parseError : "gpio switch config is invalid";
            return false;
        }
        config.switchConfig.base = base;
        size = gpioSwitchDeviceConfigSize(config);
        if (!encodeGpioSwitchDeviceConfig(config, buffer, size)) {
            error = "gpio switch config is invalid";
            return false;
        }
        break;
    }
    case 3: {
        if (configVersion != 1U) {
            error = "unsupported onewire bus config version";
            return false;
        }
        OneWireBusDeviceConfigV1 config{};
        const char* parseError = nullptr;
        if (!parseOneWireBusDeviceConfigJson(input, config, parseError)) {
            error = parseError != nullptr ? parseError : "onewire bus config is invalid";
            return false;
        }
        config.base = base;
        size = oneWireBusDeviceConfigSize(config);
        if (!encodeOneWireBusDeviceConfig(config, buffer, size)) {
            error = "onewire bus config is invalid";
            return false;
        }
        break;
    }
    case 4: {
        if (configVersion != 1U) {
            error = "unsupported ds18b20 config version";
            return false;
        }
        Ds18b20TemperatureSensorConfigV1 config{};
        const char* parseError = nullptr;
        if (!parseDs18b20TemperatureSensorConfigJson(input, config, parseError)) {
            error = parseError != nullptr ? parseError : "ds18b20 config is invalid";
            return false;
        }
        config.base = base;
        size = ds18b20TemperatureSensorConfigSize(config);
        if (!encodeDs18b20TemperatureSensorConfig(config, buffer, size)) {
            error = "ds18b20 config is invalid";
            return false;
        }
        break;
    }
    case 5: {
        if (configVersion != 1U) {
            error = "unsupported thermostat config version";
            return false;
        }
        ThermostatDeviceConfigV1 config{};
        const char* parseError = nullptr;
        if (!parseThermostatDeviceConfigJson(input, config, parseError)) {
            error = parseError != nullptr ? parseError : "thermostat config is invalid";
            return false;
        }
        config.base = base;
        size = thermostatDeviceConfigSize(config);
        if (!encodeThermostatDeviceConfig(config, buffer, size)) {
            error = "thermostat config is invalid";
            return false;
        }
        break;
    }
    default:
        error = "device bundle type is unsupported";
        return false;
    }

    if (!configBlob.assign(buffer, size)) {
        error = "device bundle config blob exceeds supported size";
        return false;
    }
    return true;
}

bool parseDeviceLine(const JsonObjectConst& input, DeviceRegistryEntry& record, DeviceConfigBlob& configBlob, std::string& error) {
    const char* kind = input["kind"] | "";
    if (std::strcmp(kind, kRecordKindDevice) != 0) {
        error = "unexpected bundle record kind";
        return false;
    }

    const JsonObjectConst recordInput = input["record"].as<JsonObjectConst>();
    const JsonObjectConst configInput = input["config"].as<JsonObjectConst>();
    const JsonObjectConst recordFields = recordInput.isNull() ? input : recordInput;
    const JsonObjectConst configFields = configInput.isNull() ? input : configInput;

    const uint32_t deviceId = recordFields["id"] | (input["id"] | (input["device_id"] | 0U));
    DeviceTypeId typeId{0};
    if (!deviceTypeIdFromTransferJson(recordFields, typeId, error)) {
        return false;
    }
    const uint32_t configRevision = recordFields["configRevision"] | (recordFields["config_revision"] | 0U);
    if (deviceId == 0U || typeId == 0U || configRevision == 0U) {
        error = "device bundle record is missing required fields";
        return false;
    }

    DeviceBaseConfigV1 base{};
    const char* baseError = nullptr;
    if (!parseDeviceBaseConfigJson(configFields, base, baseError)) {
        error = baseError != nullptr ? baseError : "device bundle base config is invalid";
        return false;
    }

    if (!encodeTransferConfigBlob(configFields, base, typeId, 1U, configBlob, error)) {
        return false;
    }

    record = {};
    record.header.recordVersion = kDeviceRecordHeaderVersion;
    record.header.deviceId = static_cast<DeviceId>(deviceId);
    record.header.typeId = static_cast<DeviceTypeId>(typeId);
    record.header.configVersion = 1U;
    record.header.configRevision = configRevision;
    record.header.payloadLength = static_cast<uint32_t>(configBlob.size());
    record.header.payloadChecksum = DeviceSetupTransferCodec::checksum(configBlob.data(), configBlob.size());
    record.persistencePolicy = DevicePersistencePolicy::Delayed;
    record.status = base.enabled != 0U ? DeviceStatus::Ready : DeviceStatus::Disabled;

    const JsonArrayConst deps = configFields["deps"].as<JsonArrayConst>();
    uint8_t depCount = 0;
    if (!deps.isNull()) {
        for (JsonVariantConst item : deps) {
            if (depCount >= kMaxDeviceDependencies) {
                error = "device bundle exceeds supported dependency count";
                return false;
            }
            const JsonObjectConst depObject = item.as<JsonObjectConst>();
            const char* roleName = depObject["role"] | "";
            DeviceDependencyRole role{DeviceDependencyRole::Unknown};
            if (!parseDeviceDependencyRole(roleName, role)) {
                error = "device bundle contains an invalid dependency role";
                return false;
            }
            const DeviceId dependencyId = static_cast<DeviceId>(depObject["deviceId"] | (depObject["device_id"] | 0U));
            if (dependencyId == 0U) {
                error = "device bundle dependency id is missing";
                return false;
            }
            record.deps[depCount++] = {role, dependencyId};
        }
    }
    record.depCount = depCount;

    return true;
}

template <typename Writer> bool writeBundleImpl(Writer& writer, const DeviceRegistry& registry, uint32_t registryRevision) {
    static_cast<void>(registryRevision);

    StaticJsonDocument<256> envelope;
    envelope["kind"] = kRecordKindEnvelope;
    envelope["transferSchemaVersion"] = DeviceSetupTransferCodec::kTransferSchemaVersion;
    envelope["registrySchemaVersion"] = kDeviceRegistrySchemaVersion;
    envelope["registryRevision"] = registryRevision;

    size_t deviceCount = 0;
    registry.forEachRuntime([&](const IDeviceRuntime& runtime) {
        (void)runtime;
        ++deviceCount;
    });
    envelope["deviceCount"] = deviceCount;

    std::string line;
    serializeJson(envelope, line);
    if (!writeLine(writer, line)) {
        return false;
    }

    bool ok = true;
    registry.forEachRuntime([&](const IDeviceRuntime& runtime) {
        if (!ok) {
            return;
        }
        StaticJsonDocument<2048> doc;
        JsonObject output = doc.to<JsonObject>();
        output["kind"] = kRecordKindDevice;
        JsonObject record = output.createNestedObject("record");
        record["id"] = runtime.deviceId();
        record["typeName"] = deviceTypeNameFromId(runtime.typeId());
        record["configRevision"] = runtime.configRevision();

        JsonObject config = output.createNestedObject("config");
        const DeviceDependencyLink* links = runtime.dependencyLinks();
        const uint8_t depCount = runtime.dependencyCount();
        JsonArray deps = config.createNestedArray("deps");
        for (uint8_t index = 0; index < depCount && links != nullptr; ++index) {
            JsonObject dep = deps.createNestedObject();
            dep["role"] = deviceDependencyRoleName(links[index].role);
            dep["deviceId"] = links[index].deviceId;
        }

        switch (runtime.typeId()) {
        case 1:
            writeDummyDeviceConfigJson(static_cast<const DummyDevice&>(runtime).config(), config);
            break;
        case 2:
            writeGpioSwitchDeviceConfigJson(static_cast<const GpioSwitchDevice&>(runtime).config(), config);
            break;
        case 3:
            writeOneWireBusDeviceConfigJson(static_cast<const OneWireBusDevice&>(runtime).config(), config);
            break;
        case 4:
            writeDs18b20TemperatureSensorConfigJson(static_cast<const Ds18b20TemperatureSensorDevice&>(runtime).config(), config);
            break;
        case 5:
            writeThermostatDeviceConfigJson(static_cast<const ThermostatDevice&>(runtime).config(), config);
            break;
        default:
            break;
        }

        line.clear();
        serializeJson(doc, line);
        if (!writeLine(writer, line)) {
            ok = false;
        }
    });
    return ok;
}

} // namespace

std::string DeviceSetupTransferCodec::encodeHex(const uint8_t* data, const size_t size) {
    static constexpr char kHexDigits[] = "0123456789abcdef";
    std::string hex;
    hex.reserve(size * 2U);
    for (size_t index = 0; index < size; ++index) {
        const uint8_t byte = data[index];
        hex.push_back(kHexDigits[(byte >> 4U) & 0x0FU]);
        hex.push_back(kHexDigits[byte & 0x0FU]);
    }
    return hex;
}

bool DeviceSetupTransferCodec::decodeHex(const std::string& hex, std::vector<uint8_t>& bytes) {
    auto hexDigit = [](char ch) -> int {
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
    };

    if ((hex.size() % 2U) != 0U) {
        return false;
    }
    bytes.clear();
    bytes.reserve(hex.size() / 2U);
    for (size_t index = 0; index < hex.size(); index += 2U) {
        const int hi = hexDigit(hex[index]);
        const int lo = hexDigit(hex[index + 1U]);
        if (hi < 0 || lo < 0) {
            return false;
        }
        bytes.push_back(static_cast<uint8_t>((hi << 4U) | lo));
    }
    return true;
}

uint32_t DeviceSetupTransferCodec::checksum(const uint8_t* data, const size_t size) {
    uint32_t hash = 2166136261UL;
    for (size_t index = 0; index < size; ++index) {
        hash ^= data[index];
        hash *= 16777619UL;
    }
    return hash;
}

#if defined(ARDUINO) && !defined(UNIT_TEST)
bool DeviceSetupTransferCodec::writeBundle(AsyncResponseStream& out, const DeviceRegistry& registry, uint32_t registryRevision) {
    StreamWriter writer{out};
    return writeBundleImpl(writer, registry, registryRevision);
}
#endif

bool DeviceSetupTransferCodec::writeBundle(std::string& out, const DeviceRegistry& registry, uint32_t registryRevision) {
    StringWriter writer{out};
    out.clear();
    return writeBundleImpl(writer, registry, registryRevision);
}

DeviceSetupTransferCodec::ParseResult DeviceSetupTransferCodec::parseFile(const char* path, const size_t fileSize) {
    ParseResult result{};
    if (path == nullptr || path[0] == '\0') {
        result.validation = {DeviceError::InvalidConfig, "bundle file path is missing"};
        return result;
    }
    if (fileSize == 0U || fileSize > kMaxBundleBytes) {
        result.validation = {DeviceError::BoundsExceeded, "bundle file exceeds supported size"};
        return result;
    }

    std::string line;
    bool envelopeSeen = false;
    DeviceRegistrySnapshot snapshot{};
    DeviceConfigBlobMap configBlobs{};

#if defined(ARDUINO) && !defined(UNIT_TEST)
    File file = LittleFS.open(path, "r");
    if (!file) {
        result.validation = {DeviceError::MissingRecord, "bundle file is missing"};
        return result;
    }
    while (file.available()) {
        const String rawLine = file.readStringUntil('\n');
        line = rawLine.c_str();
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
#else
    std::ifstream file(path);
    if (!file.is_open()) {
        result.validation = {DeviceError::MissingRecord, "bundle file is missing"};
        return result;
    }
    while (std::getline(file, line)) {
#endif
        if (line.empty()) {
            continue;
        }
        StaticJsonDocument<4096> doc;
        const DeserializationError error = deserializeJson(doc, line);
        if (error) {
            result.validation = {DeviceError::CorruptRecord, "bundle line is not valid JSON"};
            return result;
        }
        const JsonObjectConst object = doc.as<JsonObjectConst>();
        const char* kind = object["kind"] | "";
        if (!envelopeSeen) {
            if (std::strcmp(kind, kRecordKindEnvelope) != 0) {
                result.validation = {DeviceError::InvalidConfig, "bundle is missing the transfer envelope"};
                return result;
            }
            const uint32_t schemaVersion = object["transferSchemaVersion"] | (object["transfer_schema_version"] | 0U);
            if (schemaVersion != kTransferSchemaVersion) {
                result.validation = {DeviceError::InvalidVersion, "unsupported transfer schema version"};
                return result;
            }
            result.registryRevision = object["registryRevision"] | (object["registry_revision"] | 0U);
            const uint32_t expectedCount = object["deviceCount"] | (object["device_count"] | 0U);
            envelopeSeen = true;
            result.deviceCount = expectedCount;
            continue;
        }

        if (std::strcmp(kind, kRecordKindDevice) != 0) {
            result.validation = {DeviceError::InvalidConfig, "unexpected bundle record kind"};
            return result;
        }

        DeviceRegistryEntry record{};
        DeviceConfigBlob configBlob{};
        std::string errorMessage;
        if (!parseDeviceLine(object, record, configBlob, errorMessage)) {
            result.validation = {DeviceError::InvalidConfig, errorMessage.c_str()};
            return result;
        }

        snapshot.records.push_back(record);
        snapshot.indexEntries.push_back({record.header.deviceId, record.header.typeId});
        configBlobs[record.header.deviceId] = configBlob;
    }

    if (!envelopeSeen) {
        result.validation = {DeviceError::InvalidConfig, "bundle is missing the transfer envelope"};
        return result;
    }

    if (result.deviceCount != snapshot.records.size()) {
        result.validation = {DeviceError::InvalidConfig, "bundle device count mismatch"};
        return result;
    }

    result.snapshot = std::move(snapshot);
    result.configBlobs = std::move(configBlobs);
    return result;
}

} // namespace ewfm
