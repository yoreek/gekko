#include "integrations/rest/spi_bus/SpiBusDeviceApiAdapter.h"

namespace ewfm {

void SpiBusDeviceApiAdapter::writeRuntimeJson(const SpiBusDevice& device, JsonObject runtimeJson) const {
    runtimeJson["generation"] = device.generation();
    runtimeJson["transactionActive"] = device.dependencyTransactionActive();
    device.diagnostics().writeJson(runtimeJson);

    const SpiProbeResult& probe = device.probe();
    if (probe.ready) {
        JsonObject probeJson = runtimeJson.createNestedObject("probe");
        probeJson["csPin"] = probe.csPin;
        probeJson["ready"] = probe.ready;
        probeJson["checkedAtMs"] = probe.checkedAtMs;

        const char* outcomeStr = "unknown";
        switch (probe.outcome) {
        case SpiProbeOutcome::Detected:
            outcomeStr = "detected";
            break;
        case SpiProbeOutcome::NotDetected:
            outcomeStr = "not_detected";
            break;
        case SpiProbeOutcome::Inconclusive:
            outcomeStr = "inconclusive";
            break;
        case SpiProbeOutcome::Unknown:
        default:
            outcomeStr = "unknown";
            break;
        }
        probeJson["outcome"] = outcomeStr;

        const char* methodStr = "none";
        switch (probe.method) {
        case SpiProbeMethod::MisoActivity:
            methodStr = "miso_activity";
            break;
        case SpiProbeMethod::CsPullHeuristic:
            methodStr = "cs_pull_heuristic";
            break;
        case SpiProbeMethod::None:
        default:
            methodStr = "none";
            break;
        }
        probeJson["method"] = methodStr;
    }
}

} // namespace ewfm
