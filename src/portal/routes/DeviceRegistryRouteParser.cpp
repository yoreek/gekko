#include "portal/routes/DeviceRegistryRouteParser.h"

#if defined(ARDUINO) && !defined(UNIT_TEST)
#include <ESPAsyncWebServer.h>
#include <cstdlib>
#endif

namespace ewfm {

DeviceRegistryRouteParser::DeviceRegistryRouteParser(DeviceRegistry& registry, const DeviceApiAdapterRegistry& adapters)
    : registry_(registry), adapters_(adapters) {}

#if defined(ARDUINO) && !defined(UNIT_TEST)
bool DeviceRegistryRouteParser::parseDeviceId(const AsyncWebServerRequest* request, DeviceId& deviceId) const {
    if (request == nullptr) {
        return false;
    }
    const String url = request->url();
    constexpr size_t kPrefixLen = sizeof("/api/devices/") - 1;
    if (url.length() <= kPrefixLen) {
        return false;
    }

    const char* value = url.c_str() + kPrefixLen;
    char* end = nullptr;
    const unsigned long parsed = strtoul(value, &end, 10);
    if (end == value || parsed == 0UL || (end != nullptr && *end != '\0')) {
        return false;
    }
    deviceId = static_cast<DeviceId>(parsed);
    return true;
}

const DeviceRecord* DeviceRegistryRouteParser::findRecord(const AsyncWebServerRequest* request) const {
    DeviceId deviceId{0};
    if (!parseDeviceId(request, deviceId)) {
        return nullptr;
    }
    return registry_.find(deviceId);
}

const IDeviceApiAdapter* DeviceRegistryRouteParser::findAdapterForRecord(const DeviceRecord& record) const {
    return adapters_.find(record.header.typeId);
}

const IDeviceApiAdapter* DeviceRegistryRouteParser::findAdapterForCreate(const JsonVariant& json, std::string& error) const {
    if (json.isNull()) {
        error = "device payload is missing";
        return nullptr;
    }

    const JsonObjectConst object = json.as<JsonObjectConst>();
    const uint32_t typeId = object["type_id"] | 0U;
    if (typeId != 0U) {
        const IDeviceApiAdapter* adapter = adapters_.find(static_cast<DeviceTypeId>(typeId));
        if (adapter == nullptr) {
            error = "unsupported device type";
        }
        return adapter;
    }

    const char* typeName = object["type"] | "";
    const IDeviceApiAdapter* adapter = adapters_.findByName(typeName);
    if (adapter == nullptr) {
        error = "unsupported device type";
    }
    return adapter;
}
#endif

} // namespace ewfm
