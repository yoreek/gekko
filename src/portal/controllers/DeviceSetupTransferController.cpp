#include "portal/controllers/DeviceSetupTransferController.h"

#include "devices/registry/DeviceSetupTransferCodec.h"

#if defined(ARDUINO) && !defined(UNIT_TEST)
#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#endif

namespace ewfm {

namespace {
#if defined(ARDUINO) && !defined(UNIT_TEST)
const char* errorCodeForTransferError(const DeviceError error) {
    switch (error) {
    case DeviceError::UnsupportedType:
        return "UNSUPPORTED_TYPE";
    case DeviceError::InvalidDeviceId:
        return "INVALID_DEVICE_ID";
    case DeviceError::DuplicateDeviceId:
        return "DUPLICATE_DEVICE_ID";
    case DeviceError::InvalidRelationship:
        return "INVALID_RELATIONSHIP";
    case DeviceError::BoundsExceeded:
        return "BOUNDS_EXCEEDED";
    case DeviceError::StorageError:
        return "STORAGE_ERROR";
    case DeviceError::InvalidVersion:
        return "INVALID_VERSION";
    case DeviceError::CorruptRecord:
        return "BAD_JSON";
    case DeviceError::MissingRecord:
        return "NOT_FOUND";
    case DeviceError::InvalidCommand:
        return "INVALID_COMMAND";
    case DeviceError::InvalidConfig:
    case DeviceError::None:
    default:
        return "BAD_ARGS";
    }
}
#endif
} // namespace

DeviceSetupTransferController::DeviceSetupTransferController(AsyncWebServerRequest* request, const Action action, DeviceRegistry& registry)
    : BaseController(request, action), registry_(registry) {}

#if defined(ARDUINO) && !defined(UNIT_TEST)
void DeviceSetupTransferController::registerRoutes(AsyncWebServer& server, DeviceRegistry& registry) {
    server.on("/api/device-setup/export", HTTP_GET,
              [&registry](AsyncWebServerRequest* request) { DeviceSetupTransferController(request, Action::Show, registry).dispatch(); });
    server.on(
        "/api/device-setup/import", HTTP_POST,
        [&registry](AsyncWebServerRequest* request) { DeviceSetupTransferController(request, Action::Create, registry).dispatch(); },
        nullptr,
        [&registry](AsyncWebServerRequest* request, const String& filename, size_t index, uint8_t* data, size_t len, bool final) {
            if (index == 0U) {
                size_t fileIndex = 0U;
                if (!BaseController::beginUploadFile(request, filename.c_str(), "application/x-ndjson", 0U, fileIndex)) {
                    return;
                }
            }

            const size_t fileIndex = 0U;
            const size_t total = index + len;
            if (!BaseController::appendUploadFile(request, fileIndex, data, len, index, total)) {
                return;
            }
            if (final) {
                (void)BaseController::finishUploadFile(request, fileIndex, true);
            }
        });
    server.on("/api/device-setup/export", HTTP_OPTIONS, [&registry](AsyncWebServerRequest* request) {
        DeviceSetupTransferController(request, Action::Options, registry).dispatch();
    });
    server.on("/api/device-setup/import", HTTP_OPTIONS, [&registry](AsyncWebServerRequest* request) {
        DeviceSetupTransferController(request, Action::Options, registry).dispatch();
    });
}
#endif

const BaseController::RulesChain* DeviceSetupTransferController::beforeChain() {
#if defined(ARDUINO) && !defined(UNIT_TEST)
    static constexpr HookRule rules[] = {
        {&BaseController::beforeCorsOptions, ALL},
    };
    static const RulesChain node{rules, sizeof(rules) / sizeof(rules[0]), nullptr};
    return &node;
#else
    return BaseController::beforeChain();
#endif
}

void DeviceSetupTransferController::show() {
#if defined(ARDUINO) && !defined(UNIT_TEST)
    AsyncResponseStream* stream = request_->beginResponseStream("application/x-ndjson");
    if (stream == nullptr) {
        renderError(500, "INTERNAL", "failed to create export response");
        return;
    }

    stream->addHeader("Content-Disposition", "attachment; filename=device-setup.ndjson");
    if (!DeviceSetupTransferCodec::writeBundle(*stream, registry_, registry_.registryRevision())) {
        renderError(500, "INTERNAL", "failed to export device setup");
        return;
    }
    send(stream);
#endif
}

void DeviceSetupTransferController::create() {
#if defined(ARDUINO) && !defined(UNIT_TEST)
    size_t fileCount = 0U;
    const RequestFile* files = requestFiles(request_, fileCount);
    if (files == nullptr || fileCount == 0U) {
        renderError(400, "BAD_ARGS", "bundle file is required");
        return;
    }

    const RequestFile& file = files[0];
    if (!file.present || file.tmpPath[0] == '\0') {
        renderError(400, "BAD_ARGS", "bundle file is required");
        return;
    }
    if (file.size > DeviceSetupTransferCodec::kMaxBundleBytes) {
        renderError(413, "BOUNDS_EXCEEDED", "bundle file exceeds supported size");
        return;
    }

    const DeviceSetupTransferCodec::ParseResult parsed = DeviceSetupTransferCodec::parseFile(file.tmpPath, file.size);
    if (!parsed.ok()) {
        renderError(parsed.validation.error == DeviceError::BoundsExceeded ? 413 : 400, errorCodeForTransferError(parsed.validation.error),
                    parsed.validation.message);
        return;
    }

    const DeviceValidationResult restoreResult = registry_.restore(parsed.snapshot, parsed.configBlobs, parsed.registryRevision, 0);
    if (!restoreResult.ok()) {
        renderError(restoreResult.error == DeviceError::BoundsExceeded ? 413 : 500, errorCodeForTransferError(restoreResult.error),
                    restoreResult.message);
        return;
    }

    StaticJsonDocument<256> doc;
    doc["deviceCount"] = parsed.deviceCount;
    doc["registryRevision"] = registry_.registryRevision();
    renderOk(doc);
#endif
}

void DeviceSetupTransferController::options() {
    BaseController::options();
}

} // namespace ewfm
