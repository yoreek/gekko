#include "devices/display/tm1637/Tm1637Device.h"

#include <cstring>
#include <type_traits>

namespace ewfm {

namespace {
constexpr DeviceTypeId kTm1637DeviceTypeId = 33;
constexpr uint32_t kTm1637DeviceConfigVersion = 2;
constexpr uint8_t kTm1637SupportedDigitCount = Tm1637SegmentCodec::kDigitCount;
constexpr uint8_t kTm1637MaxMetricDependencies = 16;
} // namespace

Tm1637Device::Tm1637Device(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob)
    : Tm1637Device(
          [&configBlob]() {
              Tm1637DeviceConfigV2 config{};
              (void)decodeTm1637DeviceConfig(configBlob.data(), configBlob.size(), config);
              return config;
          }(),
          defaultArduinoTm1637LineDriver()) {
    bindDeviceIdentity(record, configBlob);
}

Tm1637Device::Tm1637Device(const Tm1637DeviceConfigV2& config, ITm1637LineDriver& lineDriver)
    : DisplayDeviceBase(DisplayDeviceBase::initialState()), config_(config), lines_(lineDriver), surface_(kTm1637SupportedDigitCount) {}

Tm1637Device::~Tm1637Device() = default;

const Tm1637DeviceConfigV2& Tm1637Device::config() const {
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
    return encodeFixedConfigBlob(Tm1637DeviceConfigV2::kMagic, config_, buffer, size) && configBlob.assign(buffer, size);
}

DeviceConfigUpdatePlan Tm1637Device::planConfigUpdate(const DeviceConfigBlob& configBlob) const {
    Tm1637DeviceConfigV2 config{};
    if (!decodeTm1637DeviceConfig(configBlob.data(), configBlob.size(), config)) {
        return {};
    }

    const bool panelChanged = config.panel != config_.panel;
    const bool brightnessChanged = config.brightness != config_.brightness;
    const bool rotationChanged = config.rotation != config_.rotation;
    const bool pinsChanged = config.clkPin != config_.clkPin || config.dioPin != config_.dioPin;

    DeviceConfigUpdatePlan plan{};
    plan.endOldConfig = panelChanged || brightnessChanged || rotationChanged || pinsChanged;
    plan.resetStateMachine = plan.endOldConfig;
    return plan;
}

bool Tm1637Device::applyConfig(const DeviceConfigBlob& configBlob, const uint32_t now) {
    (void)now;
    Tm1637DeviceConfigV2 config{};
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

uint8_t Tm1637Device::digitCount() const {
    return tm1637PanelProfile(static_cast<Tm1637PanelKind>(config_.panel)).digitCount;
}

bool Tm1637Device::initializeDisplayHardware(uint32_t now) {
    (void)now;
    lastFrameValid_ = false;
    surface_.clear(0U);
    // A config migrated from V1 carries no pins, so the device faults here until the portal supplies
    // them rather than driving arbitrary GPIOs.
    pinsAcquired_ = config_.pinsConfigured() && lines_.configure(config_.clkPin, config_.dioPin);
    return pinsAcquired_;
}

void Tm1637Device::releaseDisplayHardware(uint32_t now) {
    (void)now;
    if (pinsAcquired_) {
        Tm1637Protocol protocol(lines_, config_.clkPin, config_.dioPin);
        (void)protocol.displayOff();
        lines_.release(config_.clkPin, config_.dioPin);
        pinsAcquired_ = false;
    }
    lastFrameValid_ = false;
}

bool Tm1637Device::clearDisplay(uint16_t) {
    surface_.clear(0U);
    return writeDisplayFrame();
}

bool Tm1637Device::writeDisplayFrame() {
    if (!pinsAcquired_) {
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

    Tm1637Protocol protocol(lines_, config_.clkPin, config_.dioPin);
    if (!protocol.writeFrame(encoded.data(), digitCount(), config_.brightness)) {
        // An unacknowledged frame means the chip is absent or miswired; drop the cache so the next
        // tick retries instead of assuming the display already shows these digits.
        lastFrameValid_ = false;
        return false;
    }

    lastFrameBytes_ = encoded;
    lastFrameValid_ = true;
    return true;
}

DisplayLayoutRenderResult Tm1637Device::renderDisplayFrame(const MetricValueResolver& resolver, uint32_t now) {
    if (!pinsAcquired_) {
        return {};
    }

    SegmentDisplayLayoutRenderer renderer(surface_);
    const DisplayLayoutRenderResult result = renderSession_.render(layout_, resolver, renderer, now);
    if (!result.rendered) {
        return result;
    }
    if (!writeDisplayFrame()) {
        // The render session already consumed this frame, so without invalidating it the failed
        // transfer would never be retried until the rendered content happens to change.
        invalidateDisplayRender();
        return {};
    }
    return result;
}

DeviceTypeDescriptor Tm1637Device::descriptor() {
    DeviceTypeDescriptor descriptor;
    descriptor.typeId = kTm1637DeviceTypeId;
    descriptor.name = "Tm1637Device";
    descriptor.currentConfigVersion = kTm1637DeviceConfigVersion;
    descriptor.maxDependents = kTm1637MaxMetricDependencies;
    descriptor.supportsCommands = false;
    descriptor.supportsRetainedState = false;
    descriptor.defaultPersistencePolicy = DevicePersistencePolicy::Delayed;
    descriptor.ticks100ms = true;
    descriptor.dependencyRequirements = {DeviceDependencyRequirement{DeviceRole::MetricSource, false}};
    descriptor.createRuntime = &Tm1637Device::createRuntime;
    descriptor.validateConfig = &Tm1637Device::validateConfig;
    return descriptor;
}

void Tm1637Device::claimGpioPins(DeviceId* pins) const {
    setGpioPinOwner(pins, config_.clkPin, deviceId());
    setGpioPinOwner(pins, config_.dioPin, deviceId());
}

void Tm1637Device::releaseGpioPins(DeviceId* pins) const {
    setGpioPinOwner(pins, config_.clkPin, 0);
    setGpioPinOwner(pins, config_.dioPin, 0);
}

std::unique_ptr<IDeviceRuntime> Tm1637Device::createRuntime(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob) {
    return std::unique_ptr<IDeviceRuntime>(new Tm1637Device(record, configBlob));
}

DeviceValidationResult Tm1637Device::validateConfig(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob) {
    for (uint8_t index = 0U; index < record.depCount; ++index) {
        if (record.dependencyLinks() == nullptr || record.dependencyLinks()[index].role != DeviceRole::MetricSource) {
            return {DeviceError::InvalidRelationship, "display metric dependency is invalid"};
        }
    }
    if (configBlob.size() > kMaxDeviceConfigBytes) {
        return {DeviceError::BoundsExceeded, "device config exceeds supported size"};
    }
    Tm1637DeviceConfigV2 config{};
    if (!decodeTm1637DeviceConfig(configBlob.data(), configBlob.size(), config)) {
        return {DeviceError::InvalidConfig, "device config is invalid"};
    }
    return config.validate();
}

} // namespace ewfm
