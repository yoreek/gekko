#include "devices/display/tm1637/Tm1637Device.h"

#include <cstring>
#include <type_traits>

namespace ewfm {

namespace {
constexpr DeviceTypeId kTm1637DeviceTypeId = 33;
constexpr uint32_t kTm1637DeviceConfigVersion = 1;
constexpr uint8_t kTm1637SupportedDigitCount = Tm1637SegmentCodec::kDigitCount;
} // namespace

Tm1637Device::Tm1637Device(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob)
    : Tm1637Device([&configBlob]() {
          Tm1637DeviceConfigV1 config{};
          (void)decodeTm1637DeviceConfig(configBlob.data(), configBlob.size(), config);
          return config;
      }()) {
    bindDeviceIdentity(record, configBlob);
}

Tm1637Device::Tm1637Device(const Tm1637DeviceConfigV1& config)
    : DisplayDeviceBase(DisplayDeviceBase::initialState()), config_(config), surface_(kTm1637SupportedDigitCount) {
    refreshLineCache();
}

Tm1637Device::~Tm1637Device() = default;

const Tm1637DeviceConfigV1& Tm1637Device::config() const {
    return config_;
}

const DeviceBaseConfigV1& Tm1637Device::baseConfig() const {
    return config_;
}

DisplayLayoutProfile Tm1637Device::displayProfile() const {
    const Tm1637PanelProfile& profile = tm1637PanelProfile(static_cast<Tm1637PanelKind>(config_.panel));
    return segmentDisplayLayoutProfile(profile.digitCount, profile.supportedRotationsMask);
}

bool Tm1637Device::serializeConfigBlob(DeviceConfigBlob& configBlob) const {
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    const size_t size = tm1637DeviceConfigSize(config_);
    return encodeFixedConfigBlob(Tm1637DeviceConfigV1::kMagic, config_, buffer, size) && configBlob.assign(buffer, size);
}

DeviceConfigUpdatePlan Tm1637Device::planConfigUpdate(const DeviceConfigBlob& configBlob) const {
    Tm1637DeviceConfigV1 config{};
    if (!decodeTm1637DeviceConfig(configBlob.data(), configBlob.size(), config)) {
        return {};
    }

    const bool panelChanged = config.panel != config_.panel;
    const bool brightnessChanged = config.brightness != config_.brightness;
    const bool rotationChanged = config.rotation != config_.rotation;

    DeviceConfigUpdatePlan plan{};
    plan.endOldConfig = panelChanged || brightnessChanged || rotationChanged;
    plan.resetStateMachine = plan.endOldConfig;
    return plan;
}

bool Tm1637Device::applyConfig(const DeviceConfigBlob& configBlob, const uint32_t now) {
    (void)now;
    Tm1637DeviceConfigV1 config{};
    if (!decodeTm1637DeviceConfig(configBlob.data(), configBlob.size(), config)) {
        return false;
    }
    config_ = config;
    lastFrameValid_ = false;
    invalidateDisplayRender();
    return true;
}

void Tm1637Device::writeDisplayConfigJson(JsonObject output) const {
    config_.writeJson(output);
}

void Tm1637Device::refreshLineCache() {
    clockLine_ = nullptr;
    dataLine_ = nullptr;
    if (const IDeviceRuntime* clockRuntime = dependencyRuntimeAt(0U); clockRuntime != nullptr) {
        clockLine_ = const_cast<ISwitchOutputRuntime*>(clockRuntime->switchOutputRuntime());
    }
    if (const IDeviceRuntime* dataRuntime = dependencyRuntimeAt(1U); dataRuntime != nullptr) {
        dataLine_ = const_cast<ISwitchOutputRuntime*>(dataRuntime->switchOutputRuntime());
    }
}

bool Tm1637Device::dependenciesReady() const {
    return dependencyRuntimeAt(0U) != nullptr && dependencyRuntimeAt(1U) != nullptr && clockLine_ != nullptr && dataLine_ != nullptr;
}

uint8_t Tm1637Device::digitCount() const {
    return tm1637PanelProfile(static_cast<Tm1637PanelKind>(config_.panel)).digitCount;
}

bool Tm1637Device::initializeDisplayHardware(uint32_t now) {
    (void)now;
    refreshLineCache();
    surface_.clear(0U);
    lastFrameValid_ = false;
    return dependenciesReady();
}

void Tm1637Device::releaseDisplayHardware(uint32_t now) {
    if (dependenciesReady()) {
        SwitchTm1637LineDriver lineDriver(*clockLine_, *dataLine_);
        Tm1637Protocol protocol(lineDriver);
        (void)protocol.displayOff(now);
    }
    lastFrameValid_ = false;
}

bool Tm1637Device::clearDisplay(uint16_t) {
    surface_.clear(0U);
    return writeDisplayFrame(uptime());
}

bool Tm1637Device::writeDisplayFrame(uint32_t now) {
    if (!dependenciesReady()) {
        return false;
    }

    DisplayDigitalFrame frame{};
    frame.cellCount = digitCount();
    if (!surface_.snapshot(frame)) {
        return false;
    }

    std::array<uint8_t, Tm1637SegmentCodec::kDigitCount> encoded{};
    if (!Tm1637SegmentCodec::encode(frame, config_.rotation, encoded.data(), encoded.size())) {
        return false;
    }

    if (lastFrameValid_ && std::memcmp(lastFrameBytes_.data(), encoded.data(), encoded.size()) == 0U) {
        return true;
    }

    SwitchTm1637LineDriver lineDriver(*clockLine_, *dataLine_);
    Tm1637Protocol protocol(lineDriver);
    if (!protocol.writeFrame(encoded.data(), digitCount(), config_.brightness, now)) {
        return false;
    }

    lastFrameBytes_ = encoded;
    lastFrameValid_ = true;
    return true;
}

DisplayLayoutRenderResult Tm1637Device::renderDisplayFrame(const MetricValueResolver& resolver, uint32_t now) {
    if (!dependenciesReady()) {
        return {};
    }

    SegmentDisplayLayoutRenderer renderer(surface_);
    const DisplayLayoutRenderResult result = renderSession_.render(layout_, resolver, renderer, now);
    if (!result.rendered) {
        return result;
    }
    if (!writeDisplayFrame(now)) {
        return {};
    }
    return result;
}

DeviceTypeDescriptor Tm1637Device::descriptor() {
    DeviceTypeDescriptor descriptor;
    descriptor.typeId = kTm1637DeviceTypeId;
    descriptor.name = "Tm1637Device";
    descriptor.currentConfigVersion = kTm1637DeviceConfigVersion;
    descriptor.maxDependents = 2U;
    descriptor.supportsCommands = false;
    descriptor.supportsRetainedState = false;
    descriptor.defaultPersistencePolicy = DevicePersistencePolicy::Delayed;
    descriptor.ticks100ms = true;
    descriptor.dependencyRequirements = {DeviceDependencyRequirement{DeviceRole::Switch, true}};
    descriptor.createRuntime = &Tm1637Device::createRuntime;
    descriptor.validateConfig = &Tm1637Device::validateConfig;
    return descriptor;
}

std::unique_ptr<IDeviceRuntime> Tm1637Device::createRuntime(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob) {
    return std::unique_ptr<IDeviceRuntime>(new Tm1637Device(record, configBlob));
}

DeviceValidationResult Tm1637Device::validateConfig(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob) {
    if (record.depCount != 2U || record.dependencyLinks() == nullptr) {
        return {DeviceError::InvalidRelationship, "tm1637 requires clk and dio switch dependencies"};
    }
    if (dependencyLinksHaveDuplicateDeviceIds(record.dependencyLinks(), record.depCount)) {
        return {DeviceError::InvalidRelationship, "tm1637 switch dependency device id is duplicated"};
    }
    for (uint8_t index = 0U; index < record.depCount; ++index) {
        if (record.dependencyLinks()[index].role != DeviceRole::Switch) {
            return {DeviceError::InvalidRelationship, "tm1637 requires clk and dio switch dependencies"};
        }
    }
    if (configBlob.size() > kMaxDeviceConfigBytes) {
        return {DeviceError::BoundsExceeded, "tm1637 config exceeds supported size"};
    }
    Tm1637DeviceConfigV1 config{};
    if (!decodeTm1637DeviceConfig(configBlob.data(), configBlob.size(), config)) {
        return {DeviceError::InvalidConfig, "tm1637 config is invalid"};
    }
    return config.validate();
}

void Tm1637Device::setDependencyRuntime(DeviceRole role, IDeviceRuntime* dependencyRuntime) {
    DeviceRuntimeBase::setDependencyRuntime(role, dependencyRuntime);
    refreshLineCache();
}

void Tm1637Device::setDependencyRuntimeAt(uint8_t index, IDeviceRuntime* dependencyRuntime) {
    DeviceRuntimeBase::setDependencyRuntimeAt(index, dependencyRuntime);
    refreshLineCache();
}

} // namespace ewfm
