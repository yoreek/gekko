#include "devices/pixel/PixelStripDevice.h"

#include "devices/core/ConfigCodec.h"
#include "devices/registry/DeviceRetainedDataStore.h"

#include <ArduinoJson.h>
#include <algorithm>
#include <cstring>
#include <string_view>

namespace ewfm {

#undef SM_CLASS
#define SM_CLASS PixelStripDevice

PixelStripDevice::PixelStripDevice(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob)
    : PixelStripDevice([&configBlob]() {
          PixelStripDeviceConfigV1 config{};
          (void)decodePixelStripDeviceConfig(configBlob.data(), configBlob.size(), config);
          return config;
      }()) {
    bindDeviceIdentity(record, configBlob);
}

PixelStripDevice::PixelStripDevice(const PixelStripDeviceConfigV1& config)
    : DeviceRuntimeBase((PState)&PixelStripDevice::Idle), config_(config) {}

const PixelStripDeviceConfigV1& PixelStripDevice::config() const {
    return config_;
}

const DeviceBaseConfigV1& PixelStripDevice::baseConfig() const {
    return config_;
}

bool PixelStripDevice::serializeConfigBlob(DeviceConfigBlob& configBlob) const {
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    const size_t size = pixelStripDeviceConfigSize(config_);
    return encodeFixedConfigBlob(PixelStripDeviceConfigV1::kMagic, config_, buffer, size) && configBlob.assign(buffer, size);
}

DeviceConfigUpdatePlan PixelStripDevice::planConfigUpdate(const DeviceConfigBlob& configBlob) const {
    PixelStripDeviceConfigV1 next{};
    if (!decodePixelStripDeviceConfig(configBlob.data(), configBlob.size(), next)) {
        return {};
    }
    DeviceConfigUpdatePlan plan{};
    plan.endOldConfig = next.pin != config_.pin || next.pixelCount != config_.pixelCount;
    plan.resetStateMachine = plan.endOldConfig;
    return plan;
}

bool PixelStripDevice::applyConfig(const DeviceConfigBlob& configBlob, uint32_t now) {
    (void)now;
    PixelStripDeviceConfigV1 next{};
    if (!decodePixelStripDeviceConfig(configBlob.data(), configBlob.size(), next)) {
        return false;
    }
    config_ = next;
    return true;
}

uint16_t PixelStripDevice::pixelCount() const {
    return config_.pixelCount;
}

bool PixelStripDevice::setPixel(uint16_t index, PixelColor color) {
    if (index >= config_.pixelCount || index >= kMaxPixelStripLength) {
        return false;
    }
    buffer_[index] = color;
    return true;
}

bool PixelStripDevice::fill(PixelColor color) {
    const uint16_t count = std::min<uint16_t>(config_.pixelCount, static_cast<uint16_t>(kMaxPixelStripLength));
    for (uint16_t index = 0; index < count; ++index) {
        buffer_[index] = color;
    }
    return true;
}

bool PixelStripDevice::show(uint32_t now) {
    (void)now;
    if (!hardwareReady_) {
        return false;
    }
#if defined(ARDUINO) && !defined(UNIT_TEST)
    const uint16_t count = std::min<uint16_t>(config_.pixelCount, static_cast<uint16_t>(kMaxPixelStripLength));
    for (uint16_t index = 0; index < count; ++index) {
        const PixelColor& color = buffer_[index];
        strip_.setPixelColor(index, color.r, color.g, color.b);
    }
    strip_.show();
#endif
    return true;
}

PixelColor PixelStripDevice::currentPixel(uint16_t index) const {
    if (index >= config_.pixelCount || index >= kMaxPixelStripLength) {
        return {};
    }
    return buffer_[index];
}

uint8_t PixelStripDevice::liveBrightness() const {
    return liveBrightness_;
}

bool PixelStripDevice::liveOn() const {
    return liveOn_;
}

bool PixelStripDevice::parseSetOutputBrightness(const DeviceCommand& command, uint8_t& percent) const {
    if (command.type != DeviceCommandType::SetOutput) {
        return false;
    }
    StaticJsonDocument<96> doc;
    const std::string_view payload = command.payload.view();
    if (deserializeJson(doc, payload.data(), payload.size())) {
        return false;
    }
    const JsonVariantConst value = doc.as<JsonVariantConst>();
    if (!value.is<unsigned long>() && !value.is<long>() && !value.is<int>()) {
        return false;
    }
    const long parsed = value.as<long>();
    if (parsed < 0 || parsed > 100) {
        return false;
    }
    percent = static_cast<uint8_t>(parsed);
    return true;
}

bool PixelStripDevice::parseSetOutputOn(const DeviceCommand& command, bool& on) const {
    if (command.type != DeviceCommandType::SetOutput) {
        return false;
    }
    StaticJsonDocument<128> doc;
    const std::string_view payload = command.payload.view();
    if (deserializeJson(doc, payload.data(), payload.size())) {
        return false;
    }
    const JsonVariantConst value = doc["on"];
    if (!value.is<bool>()) {
        return false;
    }
    on = value.as<bool>();
    return true;
}

bool PixelStripDevice::handleCommand(const DeviceCommand& command) {
    if (status_ != DeviceStatus::Ready) {
        return false;
    }
    uint8_t percent = 0U;
    if (parseSetOutputBrightness(command, percent)) {
        liveBrightness_ = percentToPixelBrightness(percent);
        applyLiveBrightness();
        (void)show(uptime());
        retainedBrightness_ = liveBrightness_;
        retainedBrightnessAvailable_ = true;
        retainedStateDirty_ = true;
        markRuntimeStateDirty();
        return true;
    }
    bool on = false;
    if (parseSetOutputOn(command, on)) {
        liveOn_ = on;
        applyLiveBrightness();
        (void)show(uptime());
        retainedOn_ = liveOn_;
        retainedOnAvailable_ = true;
        retainedStateDirty_ = true;
        markRuntimeStateDirty();
        return true;
    }
    return false;
}

bool PixelStripDevice::retainedStateDirty() const {
    return retainedStateDirty_;
}

void PixelStripDevice::clearRetainedStateDirty() {
    retainedStateDirty_ = false;
}

DeviceValidationResult PixelStripDevice::saveRetainedState(DeviceRetainedDataStore& store) const {
    if (!config_.restorePreviousState) {
        return {};
    }
    PixelStripRetainedStateV1 record{};
    record.deviceId = deviceId();
    record.brightness = retainedBrightness_;
    record.on = retainedOn_;
    return store.save(record);
}

DeviceValidationResult PixelStripDevice::loadRetainedState(DeviceRetainedDataStore& store) {
    PixelStripRetainedStateV1 record{};
    const DeviceValidationResult result = store.load(deviceId(), record);
    if (!result.ok()) {
        return result;
    }
    retainedBrightness_ = record.brightness;
    retainedBrightnessAvailable_ = true;
    retainedOn_ = record.on;
    retainedOnAvailable_ = true;
    return {};
}

DeviceTypeDescriptor PixelStripDevice::descriptor() {
    DeviceTypeDescriptor descriptor;
    descriptor.typeId = kPixelStripDeviceTypeId;
    descriptor.name = "PixelStripDevice";
    descriptor.currentConfigVersion = kPixelStripDeviceConfigVersion;
    descriptor.maxDependents = 4;
    descriptor.supportsCommands = true;
    descriptor.supportsRetainedState = true;
    descriptor.defaultPersistencePolicy = DevicePersistencePolicy::Delayed;
    descriptor.ticks100ms = true;
    descriptor.providedRoles = ProvidedRoles::of({IPixelStripRuntime::kProvidedRole});
    descriptor.createRuntime = &PixelStripDevice::createRuntime;
    descriptor.validateConfig = &PixelStripDevice::validateConfig;
    return descriptor;
}

void PixelStripDevice::claimGpioPins(DeviceId* pins) const {
    setGpioPinOwner(pins, config_.pin, deviceId());
}

void PixelStripDevice::releaseGpioPins(DeviceId* pins) const {
    setGpioPinOwner(pins, config_.pin, 0);
}

std::unique_ptr<IDeviceRuntime> PixelStripDevice::createRuntime(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob) {
    return std::unique_ptr<IDeviceRuntime>(new PixelStripDevice(record, configBlob));
}

DeviceValidationResult PixelStripDevice::validateConfig(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob) {
    (void)record;
    PixelStripDeviceConfigV1 config{};
    if (!decodePixelStripDeviceConfig(configBlob.data(), configBlob.size(), config)) {
        return {DeviceError::InvalidConfig, "pixel strip config is invalid"};
    }
    return config.validate();
}

DeviceValidationResult PixelStripDevice::initializeHardware(uint32_t now) {
    (void)now;
    const DeviceValidationResult validation = config_.validate();
    if (!validation.ok()) {
        return validation;
    }
    liveBrightness_ = (config_.restorePreviousState && retainedBrightnessAvailable_) ? retainedBrightness_ : config_.startupBrightness;
    liveOn_ = (config_.restorePreviousState && retainedOnAvailable_) ? retainedOn_ : true;
#if defined(ARDUINO) && !defined(UNIT_TEST)
    strip_.updateLength(config_.pixelCount);
    strip_.updateType(NEO_GRB + NEO_KHZ800);
    strip_.setPin(config_.pin);
    strip_.begin();
    strip_.setBrightness(liveOn_ ? liveBrightness_ : 0U);
#endif
    hardwareReady_ = true;
    return {};
}

void PixelStripDevice::applyLiveBrightness() {
#if defined(ARDUINO) && !defined(UNIT_TEST)
    if (hardwareReady_) {
        // liveOn_ gates the hardware brightness independently of liveBrightness_: off always shows
        // black regardless of the configured brightness, on shows the actual liveBrightness_.
        strip_.setBrightness(liveOn_ ? liveBrightness_ : 0U);
    }
#endif
}

void PixelStripDevice::releaseHardware() {
#if defined(ARDUINO) && !defined(UNIT_TEST)
    if (hardwareReady_) {
        strip_.updateLength(0);
    }
#endif
    hardwareReady_ = false;
}

SM_STATE(PixelStripDevice::Idle) {
    status_ = DeviceStatus::Creating;
    if (deleteRequested_) {
        status_ = DeviceStatus::Deleting;
        setDeleted();
        releaseHardware();
        SM_GOTO(Deleting);
    }
    if (disableRequested_ || config_.enabled == 0U) {
        releaseHardware();
        status_ = DeviceStatus::Disabled;
        SM_GOTO(Disabled);
    }
    if (reconfigureRequested_) {
        status_ = DeviceStatus::Reconfiguring;
        SM_GOTO(Reconfiguring);
    }
    if (startRequested_) {
        SM_GOTO(Starting);
    }
}

SM_STATE(PixelStripDevice::Starting) {
    status_ = DeviceStatus::Starting;
    if (deleteRequested_) {
        status_ = DeviceStatus::Deleting;
        setDeleted();
        releaseHardware();
        SM_GOTO(Deleting);
    }
    if (disableRequested_ || config_.enabled == 0U) {
        releaseHardware();
        status_ = DeviceStatus::Disabled;
        SM_GOTO(Disabled);
    }
    if (reconfigureRequested_) {
        status_ = DeviceStatus::Reconfiguring;
        SM_GOTO(Reconfiguring);
    }
    if (!initializeHardware(uptime()).ok()) {
        status_ = DeviceStatus::Faulted;
        requestFault();
        SM_GOTO(Faulted);
    }

    clearStartRequested();
    status_ = DeviceStatus::Ready;
    SM_GOTO(Ready);
}

SM_STATE(PixelStripDevice::Reconfiguring) {
    status_ = DeviceStatus::Reconfiguring;
    clearReconfigureRequested();
    releaseHardware();
    if (deleteRequested_) {
        status_ = DeviceStatus::Deleting;
        setDeleted();
        SM_GOTO(Deleting);
    }
    if (disableRequested_ || config_.enabled == 0U) {
        status_ = DeviceStatus::Disabled;
        SM_GOTO(Disabled);
    }
    if (!initializeHardware(uptime()).ok()) {
        status_ = DeviceStatus::Faulted;
        requestFault();
        SM_GOTO(Faulted);
    }
    status_ = DeviceStatus::Ready;
    markRuntimeStateDirty();
    SM_GOTO(Ready);
}

SM_STATE(PixelStripDevice::Ready) {
    status_ = DeviceStatus::Ready;
    if (deleteRequested_) {
        status_ = DeviceStatus::Deleting;
        setDeleted();
        releaseHardware();
        SM_GOTO(Deleting);
    }
    if (disableRequested_ || config_.enabled == 0U) {
        releaseHardware();
        status_ = DeviceStatus::Disabled;
        SM_GOTO(Disabled);
    }
    if (reconfigureRequested_) {
        status_ = DeviceStatus::Reconfiguring;
        SM_GOTO(Reconfiguring);
    }
    if (faultRequested_) {
        status_ = DeviceStatus::Faulted;
        SM_GOTO(Faulted);
    }
}

SM_STATE(PixelStripDevice::Disabled) {
    status_ = DeviceStatus::Disabled;
    releaseHardware();
    if (deleteRequested_) {
        status_ = DeviceStatus::Deleting;
        setDeleted();
        SM_GOTO(Deleting);
    }
    if (!disableRequested_ && config_.enabled != 0U) {
        status_ = DeviceStatus::Reconfiguring;
        SM_GOTO(Reconfiguring);
    }
}

SM_STATE(PixelStripDevice::Faulted) {
    status_ = DeviceStatus::Faulted;
    releaseHardware();
    if (deleteRequested_) {
        status_ = DeviceStatus::Deleting;
        setDeleted();
        SM_GOTO(Deleting);
    }
    if (reconfigureRequested_) {
        clearFaultRequested();
        status_ = DeviceStatus::Reconfiguring;
        SM_GOTO(Reconfiguring);
    }
}

SM_STATE(PixelStripDevice::Deleting) {
    status_ = DeviceStatus::Deleting;
    releaseHardware();
    setDeleted();
}

} // namespace ewfm
