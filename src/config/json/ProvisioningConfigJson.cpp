#include "config/json/ProvisioningConfigJson.h"

namespace ewfm {

void ProvisioningConfigJson::write(JsonDocument& doc, const ProvisioningConfig& config) {
    JsonObject provisioning = doc.createNestedObject("provisioning");
    provisioning["http_portal_enabled"] = config.httpPortalEnabled;
    provisioning["mobile_enabled"] = config.mobileProvisioningEnabled;
    provisioning["mobile_ble_transport"] = config.mobileBleTransport;
    provisioning["reset_provisioned_on_start"] = config.resetProvisionedOnStart;
    provisioning["proof_of_possession_redacted"] = !config.proofOfPossession.empty();
    provisioning["session_timeout_ms"] = config.sessionTimeoutMs;
}

void ProvisioningConfigJson::read(JsonDocument& doc, DeviceConfig& config) {
    if (!doc["provisioning"].is<JsonObject>()) {
        return;
    }

    JsonObject provisioning = doc["provisioning"];
    config.provisioning.httpPortalEnabled = provisioning["http_portal_enabled"] | config.provisioning.httpPortalEnabled;
    config.provisioning.mobileProvisioningEnabled = provisioning["mobile_enabled"] | config.provisioning.mobileProvisioningEnabled;
    config.provisioning.mobileBleTransport = provisioning["mobile_ble_transport"] | config.provisioning.mobileBleTransport;
    config.provisioning.resetProvisionedOnStart = provisioning["reset_provisioned_on_start"] | config.provisioning.resetProvisionedOnStart;
    config.provisioning.sessionTimeoutMs = provisioning["session_timeout_ms"] | config.provisioning.sessionTimeoutMs;
}

} // namespace ewfm
