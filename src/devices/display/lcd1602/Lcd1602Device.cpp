#include "devices/display/lcd1602/Lcd1602Device.h"

#include "devices/core/ConfigCodec.h"
#include "devices/display/DisplayTextEvaluator.h"
#include "devices/display/DisplayTextPlaceholderAst.h"

#if defined(ARDUINO) && !defined(UNIT_TEST)
#include <Arduino.h>
#endif

#include <cstring>
#include <type_traits>

namespace ewfm {

#undef SM_CLASS
#define SM_CLASS Lcd1602Device

namespace {
constexpr DeviceTypeId kLcd1602DeviceTypeId = 28;
constexpr uint32_t kLcd1602DeviceConfigVersion = 1;

// HD44780 timing needs microsecond/millisecond hardware settle waits (enable pulse width, command
// execution time) that have nothing to do with cooperative tick scheduling -- every Arduino HD44780
// library does the same. No-op under native unit tests, which exercise the protocol logic against a
// fake IPortExpanderRuntime and don't care about real timing.
void hardwareSettleMicros(uint32_t microseconds) {
#if defined(ARDUINO) && !defined(UNIT_TEST)
    delayMicroseconds(microseconds);
#else
    (void)microseconds;
#endif
}

void hardwareSettleMillis(uint32_t milliseconds) {
#if defined(ARDUINO) && !defined(UNIT_TEST)
    delay(milliseconds);
#else
    (void)milliseconds;
#endif
}
} // namespace

static_assert(std::is_trivially_copyable<Lcd1602DeviceConfigV1>::value, "Lcd1602DeviceConfigV1 must be POD");

Lcd1602Device::Lcd1602Device(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob)
    : Lcd1602Device([&configBlob]() {
          Lcd1602DeviceConfigV1 config{};
          (void)decodeLcd1602DeviceConfig(configBlob.data(), configBlob.size(), config);
          return config;
      }()) {
    bindDeviceIdentity(record, configBlob);
}

Lcd1602Device::Lcd1602Device(const Lcd1602DeviceConfigV1& config) : DeviceRuntimeBase((PState)&Lcd1602Device::Idle), config_(config) {}

const Lcd1602DeviceConfigV1& Lcd1602Device::config() const {
    return config_;
}

const DeviceBaseConfigV1& Lcd1602Device::baseConfig() const {
    return config_;
}

const char* Lcd1602Device::renderedLine1() const {
    return hasRenderedOnce_ ? lastLine1_ : "";
}

const char* Lcd1602Device::renderedLine2() const {
    return hasRenderedOnce_ ? lastLine2_ : "";
}

bool Lcd1602Device::serializeConfigBlob(DeviceConfigBlob& configBlob) const {
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    const size_t size = lcd1602DeviceConfigSize(config_);
    return encodeFixedConfigBlob(Lcd1602DeviceConfigV1::kMagic, config_, buffer, size) && configBlob.assign(buffer, size);
}

DeviceConfigUpdatePlan Lcd1602Device::planConfigUpdate(const DeviceConfigBlob& configBlob) const {
    Lcd1602DeviceConfigV1 config{};
    if (!decodeLcd1602DeviceConfig(configBlob.data(), configBlob.size(), config)) {
        return {};
    }
    DeviceConfigUpdatePlan plan{};
    plan.endOldConfig = config.rsChannel != config_.rsChannel || config.eChannel != config_.eChannel ||
                        config.d4Channel != config_.d4Channel || config.d5Channel != config_.d5Channel ||
                        config.d6Channel != config_.d6Channel || config.d7Channel != config_.d7Channel ||
                        config.backlightChannel != config_.backlightChannel;
    plan.resetStateMachine = plan.endOldConfig;
    return plan;
}

bool Lcd1602Device::applyConfig(const DeviceConfigBlob& configBlob, uint32_t now) {
    (void)now;
    Lcd1602DeviceConfigV1 config{};
    if (!decodeLcd1602DeviceConfig(configBlob.data(), configBlob.size(), config)) {
        return false;
    }
    config_ = config;
    hasRenderedOnce_ = false;
    return true;
}

uint8_t Lcd1602Device::expanderChannels(uint8_t* out, uint8_t maxOut) const {
    return lcd1602ConfigChannels(config_, out, maxOut);
}

CharacterDisplayRuntimeBase* Lcd1602Device::characterDisplayRuntime() {
    return this;
}

IPortExpanderRuntime* Lcd1602Device::dependencyExpander() const {
    IDeviceRuntime* dependency = dependencyRuntime(DeviceRole::PortExpander);
    if (dependency == nullptr) {
        return nullptr;
    }
    return const_cast<IPortExpanderRuntime*>(dependency->portExpanderRuntime());
}

bool Lcd1602Device::writeNibble(IPortExpanderRuntime& expander, const uint8_t nibble, const bool rs, const uint32_t now) const {
    bool ok = expander.requestChannelState(config_.rsChannel, rs, now);
    ok = expander.requestChannelState(config_.d4Channel, (nibble & 0x1U) != 0U, now) && ok;
    ok = expander.requestChannelState(config_.d5Channel, (nibble & 0x2U) != 0U, now) && ok;
    ok = expander.requestChannelState(config_.d6Channel, (nibble & 0x4U) != 0U, now) && ok;
    ok = expander.requestChannelState(config_.d7Channel, (nibble & 0x8U) != 0U, now) && ok;
    ok = expander.requestChannelState(config_.eChannel, true, now) && ok;
    hardwareSettleMicros(1U);
    ok = expander.requestChannelState(config_.eChannel, false, now) && ok;
    hardwareSettleMicros(50U);
    return ok;
}

bool Lcd1602Device::writeByte(IPortExpanderRuntime& expander, const uint8_t value, const bool rs, const uint32_t now) const {
    const bool highOk = writeNibble(expander, static_cast<uint8_t>(value >> 4U), rs, now);
    const bool lowOk = writeNibble(expander, static_cast<uint8_t>(value & 0x0FU), rs, now);
    return highOk && lowOk;
}

bool Lcd1602Device::runInitSequence(const uint32_t now) {
    IPortExpanderRuntime* expander = dependencyExpander();
    if (expander == nullptr) {
        return false;
    }

    bool ok = true;
    if (config_.backlightChannel != kLcd1602ChannelUnset) {
        ok = expander->requestChannelState(config_.backlightChannel, true, now) && ok;
    }

    // HD44780 4-bit initialization by instruction (datasheet "Initializing by Instruction"): force
    // 8-bit mode three times regardless of the controller's current (unknown) state, then switch to
    // 4-bit mode, then the usual function-set/display-on/entry-mode/clear command sequence.
    ok = writeNibble(*expander, 0x03U, false, now) && ok;
    hardwareSettleMillis(5U);
    ok = writeNibble(*expander, 0x03U, false, now) && ok;
    hardwareSettleMicros(150U);
    ok = writeNibble(*expander, 0x03U, false, now) && ok;
    hardwareSettleMicros(150U);
    ok = writeNibble(*expander, 0x02U, false, now) && ok;
    hardwareSettleMicros(150U);

    ok = writeByte(*expander, 0x28U, false, now) && ok; // function set: 4-bit, 2 line, 5x8 font
    ok = writeByte(*expander, 0x0CU, false, now) && ok; // display on, cursor off, blink off
    ok = writeByte(*expander, 0x06U, false, now) && ok; // entry mode: increment, no shift
    ok = writeByte(*expander, 0x01U, false, now) && ok; // clear display
    hardwareSettleMillis(2U);
    return ok;
}

bool Lcd1602Device::writeLine(IPortExpanderRuntime& expander, const uint8_t row, const char* text, const uint32_t now) const {
    const uint8_t addr = row == 0U ? 0x00U : 0x40U;
    bool ok = writeByte(expander, static_cast<uint8_t>(0x80U | addr), false, now);
    for (size_t i = 0; i < kLcd1602LineLength; ++i) {
        ok = writeByte(expander, static_cast<uint8_t>(text[i]), true, now) && ok;
    }
    return ok;
}

void Lcd1602Device::resolveLine(const char* templateText, const MetricValueResolver& resolver, char (&out)[kLcd1602LineLength + 1U]) const {
    const DisplayTextCompileResult compiled = compileDisplayTextWidget(templateText);
    DisplayTextEvaluationResult evaluated{};
    const bool resolved = compiled.ok() && evaluateDisplayTextWidget(templateText, compiled.compiled, resolver, evaluated);
    const char* source = resolved ? evaluated.text : templateText;

    size_t length = 0U;
    while (length < kLcd1602LineLength && source[length] != '\0') {
        out[length] = source[length];
        ++length;
    }
    for (size_t i = length; i < kLcd1602LineLength; ++i) {
        out[i] = ' ';
    }
    out[kLcd1602LineLength] = '\0';
}

bool Lcd1602Device::renderText(const MetricValueResolver& resolver, const uint32_t now) {
    if (status_ != DeviceStatus::Ready) {
        return false;
    }
    IPortExpanderRuntime* expander = dependencyExpander();
    if (expander == nullptr) {
        return false;
    }

    char resolvedLine1[kLcd1602LineLength + 1U];
    char resolvedLine2[kLcd1602LineLength + 1U];
    resolveLine(config_.line1, resolver, resolvedLine1);
    resolveLine(config_.line2, resolver, resolvedLine2);

    bool changed = false;
    if (!hasRenderedOnce_ || std::strcmp(resolvedLine1, lastLine1_) != 0) {
        if (!writeLine(*expander, 0U, resolvedLine1, now)) {
            return false;
        }
        std::memcpy(lastLine1_, resolvedLine1, sizeof(lastLine1_));
        changed = true;
    }
    if (!hasRenderedOnce_ || std::strcmp(resolvedLine2, lastLine2_) != 0) {
        if (!writeLine(*expander, 1U, resolvedLine2, now)) {
            return false;
        }
        std::memcpy(lastLine2_, resolvedLine2, sizeof(lastLine2_));
        changed = true;
    }
    hasRenderedOnce_ = true;
    return changed;
}

DeviceTypeDescriptor Lcd1602Device::descriptor() {
    DeviceTypeDescriptor descriptor;
    descriptor.typeId = kLcd1602DeviceTypeId;
    descriptor.name = "Lcd1602Device";
    descriptor.currentConfigVersion = kLcd1602DeviceConfigVersion;
    descriptor.maxDependents = 0;
    descriptor.supportsCommands = false;
    descriptor.supportsRetainedState = false;
    descriptor.defaultPersistencePolicy = DevicePersistencePolicy::Delayed;
    descriptor.ticks100ms = true;
    descriptor.dependencyRequirements = {
        {DeviceRole::PortExpander, true},
        {DeviceRole::MetricSource, false},
    };
    descriptor.createRuntime = &Lcd1602Device::createRuntime;
    descriptor.validateConfig = &Lcd1602Device::validateConfig;
    return descriptor;
}

std::unique_ptr<IDeviceRuntime> Lcd1602Device::createRuntime(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob) {
    return std::unique_ptr<IDeviceRuntime>(new Lcd1602Device(record, configBlob));
}

DeviceValidationResult Lcd1602Device::validateConfig(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob) {
    if (record.dependencyDeviceId(DeviceRole::PortExpander) == 0U) {
        return {DeviceError::InvalidRelationship, "lcd1602 requires a port expander dependency"};
    }
    Lcd1602DeviceConfigV1 config{};
    if (!decodeLcd1602DeviceConfig(configBlob.data(), configBlob.size(), config)) {
        return {DeviceError::CorruptRecord, "lcd1602 config is invalid"};
    }
    return config.validate();
}

SM_STATE(Lcd1602Device::Idle) {
    status_ = DeviceStatus::Creating;
    if (deleteRequested_) {
        status_ = DeviceStatus::Deleting;
        setDeleted();
        SM_GOTO(Deleting);
    }
    if (disableRequested_ || !enabled()) {
        status_ = DeviceStatus::Disabled;
        SM_GOTO(Disabled);
    }
    if (startRequested_) {
        SM_GOTO(Starting);
    }
}

SM_STATE(Lcd1602Device::Starting) {
    status_ = DeviceStatus::Starting;
    if (deleteRequested_) {
        status_ = DeviceStatus::Deleting;
        setDeleted();
        SM_GOTO(Deleting);
    }
    if (disableRequested_ || !enabled()) {
        status_ = DeviceStatus::Disabled;
        SM_GOTO(Disabled);
    }
    if (!dependenciesReady()) {
        status_ = DeviceStatus::DependencyBlocked;
        return;
    }
    if (reconfigureRequested_) {
        status_ = DeviceStatus::Reconfiguring;
        SM_GOTO(Reconfiguring);
    }
    if (!runInitSequence(uptime())) {
        status_ = DeviceStatus::Faulted;
        SM_GOTO(Faulted);
    }
    hasRenderedOnce_ = false;
    startRequested_ = false;
    status_ = DeviceStatus::Ready;
    SM_GOTO(Ready);
}

SM_STATE(Lcd1602Device::Ready) {
    status_ = DeviceStatus::Ready;
    if (deleteRequested_) {
        status_ = DeviceStatus::Deleting;
        setDeleted();
        SM_GOTO(Deleting);
    }
    if (disableRequested_ || !enabled()) {
        status_ = DeviceStatus::Disabled;
        SM_GOTO(Disabled);
    }
    if (!dependenciesReady()) {
        SM_GOTO(Starting);
    }
    if (reconfigureRequested_) {
        status_ = DeviceStatus::Reconfiguring;
        SM_GOTO(Reconfiguring);
    }
}

SM_STATE(Lcd1602Device::Reconfiguring) {
    status_ = DeviceStatus::Reconfiguring;
    reconfigureRequested_ = false;
    SM_GOTO(Starting);
}

SM_STATE(Lcd1602Device::Disabled) {
    status_ = DeviceStatus::Disabled;
    if (deleteRequested_) {
        status_ = DeviceStatus::Deleting;
        setDeleted();
        SM_GOTO(Deleting);
    }
    if (reconfigureRequested_) {
        status_ = DeviceStatus::Reconfiguring;
        SM_GOTO(Reconfiguring);
    }
}

SM_STATE(Lcd1602Device::Faulted) {
    status_ = DeviceStatus::Faulted;
    if (deleteRequested_) {
        status_ = DeviceStatus::Deleting;
        setDeleted();
        SM_GOTO(Deleting);
    }
    if (disableRequested_ || !enabled()) {
        status_ = DeviceStatus::Disabled;
        SM_GOTO(Disabled);
    }
    if (reconfigureRequested_) {
        status_ = DeviceStatus::Reconfiguring;
        SM_GOTO(Reconfiguring);
    }
}

SM_STATE(Lcd1602Device::Deleting) {
    status_ = DeviceStatus::Deleting;
    setDeleted();
}

} // namespace ewfm
