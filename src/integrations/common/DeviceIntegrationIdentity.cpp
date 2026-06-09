#include "integrations/common/DeviceIntegrationIdentity.h"

#include <cctype>
#include <cstdio>

namespace ewfm {

namespace {
std::string sanitizeControllerIdentity(const std::string& value) {
    std::string out;
    out.reserve(value.size());
    for (char ch : value) {
        const unsigned char uch = static_cast<unsigned char>(ch);
        if (std::isalnum(uch)) {
            out.push_back(static_cast<char>(std::tolower(uch)));
            continue;
        }
        if (ch == '-' || ch == '_') {
            out.push_back(ch);
            continue;
        }
        out.push_back('_');
    }

    while (!out.empty() && out.front() == '_') {
        out.erase(out.begin());
    }
    while (!out.empty() && out.back() == '_') {
        out.pop_back();
    }

    if (out.empty()) {
        return "controller";
    }
    return out;
}
} // namespace

std::string makeExternalDeviceId(const std::string& controllerIdentity, DeviceId deviceId) {
    char suffix[16];
    std::snprintf(suffix, sizeof(suffix), "%08x", static_cast<unsigned>(deviceId));
    return sanitizeControllerIdentity(controllerIdentity) + "-dev-" + suffix;
}

} // namespace ewfm
