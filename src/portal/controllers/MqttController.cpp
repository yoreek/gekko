#include "portal/controllers/MqttController.h"

#include "integrations/mqtt/HaDiscoveryBridge.h"

#include <vector>

#if defined(ARDUINO) && !defined(UNIT_TEST)
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#endif

namespace ewfm {

namespace {
#if defined(ARDUINO) && !defined(UNIT_TEST)
bool readFileBytes(const char* path, size_t size, std::vector<uint8_t>& out) {
    File file = LittleFS.open(path, "r");
    if (!file) {
        return false;
    }
    out.resize(size);
    const size_t bytesRead = file.read(out.data(), size);
    file.close();
    return bytesRead == size;
}
#endif
} // namespace

MqttController::MqttController(AsyncWebServerRequest* request, const Action action, MqttConfigStore* store, MqttManager* manager,
                               HaDiscoveryBridge* haDiscoveryBridge)
    : BaseController(request, action), store_(store), manager_(manager), haDiscoveryBridge_(haDiscoveryBridge) {}

#if defined(ARDUINO) && !defined(UNIT_TEST)
void MqttController::registerRoutes(AsyncWebServer& server, MqttConfigStore* store, MqttManager* manager,
                                    HaDiscoveryBridge* haDiscoveryBridge) {
#if defined(WITH_HOME_ASSISTANT)
    if (store == nullptr || manager == nullptr) {
        return;
    }
    server.on(AsyncURIMatcher::exact("/api/mqtt/status"), HTTP_GET,
              [store, manager](AsyncWebServerRequest* request) { MqttController(request, Action::Show, store, manager).dispatch(); });
    server.on(AsyncURIMatcher::exact("/api/mqtt/status"), HTTP_OPTIONS,
              [store, manager](AsyncWebServerRequest* request) { MqttController(request, Action::Options, store, manager).dispatch(); });

    server.on(AsyncURIMatcher::exact("/api/mqtt/settings"), HTTP_GET,
              [store, manager](AsyncWebServerRequest* request) { MqttController(request, Action::Index, store, manager).dispatch(); });
    server.on(
        AsyncURIMatcher::exact("/api/mqtt/settings"), HTTP_PUT, [](AsyncWebServerRequest* request) { (void)request; }, nullptr,
        [store, manager, haDiscoveryBridge](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
            if (!BaseController::appendRequestBody(request, data, len, index, total)) {
                return;
            }
            MqttController(request, Action::Update, store, manager, haDiscoveryBridge)
                .dispatch(static_cast<uint8_t*>(request->_tempObject), total);
            BaseController::clearRequestBody(request);
        });
    server.on(AsyncURIMatcher::exact("/api/mqtt/settings"), HTTP_OPTIONS,
              [store, manager](AsyncWebServerRequest* request) { MqttController(request, Action::Options, store, manager).dispatch(); });

    server.on(
        AsyncURIMatcher::exact("/api/mqtt/ca-cert"), HTTP_POST,
        [store, manager](AsyncWebServerRequest* request) { MqttController(request, Action::Create, store, manager).dispatch(); },
        [](AsyncWebServerRequest* request, const String& filename, size_t index, uint8_t* data, size_t len, bool final) {
            size_t fileIndex = 0U;
            if (index == 0U) {
                if (!BaseController::beginUploadFile(request, filename.c_str(), "application/x-pem-file", 0U, fileIndex)) {
                    return;
                }
            }
            const size_t total = index + len;
            if (!BaseController::appendUploadFile(request, fileIndex, data, len, index, total)) {
                return;
            }
            if (final) {
                (void)BaseController::finishUploadFile(request, fileIndex, true);
            }
        },
        nullptr);
    server.on(AsyncURIMatcher::exact("/api/mqtt/ca-cert"), HTTP_DELETE,
              [store, manager](AsyncWebServerRequest* request) { MqttController(request, Action::Destroy, store, manager).dispatch(); });
    server.on(AsyncURIMatcher::exact("/api/mqtt/ca-cert"), HTTP_OPTIONS,
              [store, manager](AsyncWebServerRequest* request) { MqttController(request, Action::Options, store, manager).dispatch(); });
#else
    (void)server;
    (void)store;
    (void)manager;
    (void)haDiscoveryBridge;
#endif
}
#endif

const BaseController::RulesChain* MqttController::beforeChain() {
    static constexpr HookRule rules[] = {
        {MqttController::parseUpdateBody, A(Action::Update)},
    };
    static const RulesChain node{rules, sizeof(rules) / sizeof(rules[0]), BaseController::beforeChain()};
    return &node;
}

bool MqttController::parseUpdateBody(BaseController& self) {
    return static_cast<MqttController&>(self).parseBody(1024);
}

const char* MqttController::errorCodeForMqttError(const MqttConfigError error) {
    switch (error) {
    case MqttConfigError::HostRequired:
        return "HOST_REQUIRED";
    case MqttConfigError::HostTooLong:
        return "HOST_TOO_LONG";
    case MqttConfigError::PortInvalid:
        return "PORT_INVALID";
    case MqttConfigError::ClientIdRequired:
        return "CLIENT_ID_REQUIRED";
    case MqttConfigError::ClientIdTooLong:
        return "CLIENT_ID_TOO_LONG";
    case MqttConfigError::UsernameTooLong:
        return "USERNAME_TOO_LONG";
    case MqttConfigError::PasswordTooLong:
        return "PASSWORD_TOO_LONG";
    case MqttConfigError::DiscoveryPrefixInvalid:
        return "DISCOVERY_PREFIX_INVALID";
    case MqttConfigError::NodeIdRequired:
        return "NODE_ID_REQUIRED";
    case MqttConfigError::NodeIdTooLong:
        return "NODE_ID_TOO_LONG";
    case MqttConfigError::NodeIdInvalidCharacters:
        return "NODE_ID_INVALID_CHARACTERS";
    case MqttConfigError::NodeNameTooLong:
        return "NODE_NAME_TOO_LONG";
    case MqttConfigError::CertTooLarge:
        return "CERT_TOO_LARGE";
    case MqttConfigError::StorageError:
        return "STORAGE_ERROR";
    case MqttConfigError::None:
    default:
        return "BAD_ARGS";
    }
}

void MqttController::index() {
#if defined(ARDUINO) && !defined(UNIT_TEST)
    if (store_ == nullptr) {
        renderError(500, "INTERNAL", "mqtt not available");
        return;
    }
    const MqttSettings& settings = store_->settings();
    StaticJsonDocument<640> doc;
    doc["enabled"] = settings.enabled;
    doc["host"] = settings.host;
    doc["port"] = settings.port;
    doc["useTls"] = settings.useTls;
    doc["clientId"] = settings.clientId;
    doc["username"] = settings.username;
    doc["passwordRedacted"] = !settings.password.empty();
    doc["haDiscoveryPrefix"] = settings.haDiscoveryPrefix;
    doc["haNodeId"] = settings.haNodeId;
    doc["haNodeName"] = settings.haNodeName;
    doc["hasCaCert"] = store_->hasCaCert();
    renderOk(doc);
#endif
}

void MqttController::show() {
#if defined(ARDUINO) && !defined(UNIT_TEST)
    StaticJsonDocument<384> doc;
    doc["enabled"] = compiledIn();
    doc["connected"] = manager_ != nullptr && manager_->connected();
    doc["waitingForStation"] = manager_ != nullptr && manager_->waitingForStation();
    if (store_ != nullptr) {
        const MqttSettings& settings = store_->settings();
        doc["host"] = settings.host;
        doc["port"] = settings.port;
        doc["useTls"] = settings.useTls;
        doc["clientId"] = settings.clientId;
        doc["hasCaCert"] = store_->hasCaCert();
    }
    renderOk(doc);
#endif
}

void MqttController::update() {
#if defined(ARDUINO) && !defined(UNIT_TEST)
    if (store_ == nullptr || manager_ == nullptr) {
        renderError(500, "INTERNAL", "mqtt not available");
        return;
    }
    const JsonObjectConst input = getDoc()->as<JsonObjectConst>();
    MqttSettings settings = store_->settings();
    if (input["enabled"].is<bool>()) {
        settings.enabled = input["enabled"].as<bool>();
    }
    if (input["host"].is<const char*>()) {
        settings.host = input["host"].as<const char*>();
    }
    if (input["port"].is<int>()) {
        settings.port = static_cast<uint16_t>(input["port"].as<int>());
    }
    if (input["useTls"].is<bool>()) {
        settings.useTls = input["useTls"].as<bool>();
    }
    if (input["clientId"].is<const char*>()) {
        settings.clientId = input["clientId"].as<const char*>();
    }
    if (input["username"].is<const char*>()) {
        settings.username = input["username"].as<const char*>();
    }
    if (input["password"].is<const char*>()) {
        settings.password = input["password"].as<const char*>();
    }
    if (input["haDiscoveryPrefix"].is<const char*>()) {
        settings.haDiscoveryPrefix = input["haDiscoveryPrefix"].as<const char*>();
    }
    if (input["haNodeId"].is<const char*>()) {
        settings.haNodeId = input["haNodeId"].as<const char*>();
    }
    if (input["haNodeName"].is<const char*>()) {
        settings.haNodeName = input["haNodeName"].as<const char*>();
    }

    const MqttConfigValidationResult result = store_->save(settings);
    if (!result.ok()) {
        renderError(400, errorCodeForMqttError(result.error), result.message);
        return;
    }

    std::vector<uint8_t> caCert;
    store_->loadCaCert(caCert);
    manager_->applySettings(store_->settings(), caCert);
    if (haDiscoveryBridge_ != nullptr) {
        haDiscoveryBridge_->begin(store_->settings().haNodeId, store_->settings().haNodeName, store_->settings().haDiscoveryPrefix);
    }

    index();
#endif
}

void MqttController::create() {
#if defined(ARDUINO) && !defined(UNIT_TEST)
    if (store_ == nullptr || manager_ == nullptr) {
        renderError(500, "INTERNAL", "mqtt not available");
        return;
    }
    size_t fileCount = 0U;
    const RequestFile* requestFilesPtr = requestFiles(request_, fileCount);
    if (requestFilesPtr == nullptr || fileCount == 0U || !requestFilesPtr[0].present || requestFilesPtr[0].tmpPath[0] == '\0') {
        renderError(400, "BAD_ARGS", "cert file is required");
        return;
    }
    const RequestFile& file = requestFilesPtr[0];
    if (file.size == 0U || file.size > kMqttMaxCaCertBytes) {
        renderError(413, "CERT_TOO_LARGE", "ca certificate exceeds supported size");
        return;
    }

    std::vector<uint8_t> bytes;
    if (!readFileBytes(file.tmpPath, file.size, bytes)) {
        renderError(500, "INTERNAL", "failed to read uploaded certificate");
        return;
    }

    const MqttConfigValidationResult saveResult = store_->saveCaCert(bytes.data(), bytes.size());
    if (!saveResult.ok()) {
        renderError(400, errorCodeForMqttError(saveResult.error), saveResult.message);
        return;
    }

    manager_->applySettings(store_->settings(), bytes);
    show();
#endif
}

void MqttController::destroy() {
#if defined(ARDUINO) && !defined(UNIT_TEST)
    if (store_ == nullptr || manager_ == nullptr) {
        renderError(500, "INTERNAL", "mqtt not available");
        return;
    }
    store_->clearCaCert();
    manager_->applySettings(store_->settings(), {});
    show();
#endif
}

void MqttController::options() {
    BaseController::options();
}

} // namespace ewfm
